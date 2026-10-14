#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "dc_runtime.hpp"
#include "dcr_plugin_api.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <deque>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
using DcrSocket = SOCKET;
static constexpr DcrSocket kBadSocket = INVALID_SOCKET;
#else
#include <arpa/inet.h>
#include <cerrno>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <unistd.h>
using DcrSocket = int;
static constexpr DcrSocket kBadSocket = -1;
#endif

namespace dcrecomp_generated {
namespace {

inline std::uint32_t physical29(std::uint32_t address) noexcept { return address & 0x1FFFFFFFu; }
inline std::uint16_t be16(const std::uint8_t* p) noexcept { return static_cast<std::uint16_t>((p[0] << 8u) | p[1]); }
inline std::uint32_t be32(const std::uint8_t* p) noexcept { return (static_cast<std::uint32_t>(p[0]) << 24u) | (static_cast<std::uint32_t>(p[1]) << 16u) | (static_cast<std::uint32_t>(p[2]) << 8u) | p[3]; }
inline void put_be16(std::uint8_t* p, std::uint16_t v) noexcept { p[0]=static_cast<std::uint8_t>(v>>8u); p[1]=static_cast<std::uint8_t>(v); }
inline void put_be32(std::uint8_t* p, std::uint32_t v) noexcept { p[0]=static_cast<std::uint8_t>(v>>24u); p[1]=static_cast<std::uint8_t>(v>>16u); p[2]=static_cast<std::uint8_t>(v>>8u); p[3]=static_cast<std::uint8_t>(v); }

constexpr std::uint32_t kModemPhysicalBase = 0x00600000u;
constexpr std::uint32_t kModemHollyBank = 1u;
constexpr std::uint32_t kModemHollyBit = 1u << 2u; // Holly EXP_8BIT
constexpr std::uint64_t kSh4Clock = 200000000ull;
constexpr std::uint64_t kHostSocketPollCycles = kSh4Clock / 1000u; // 1 ms guest time
constexpr std::uint64_t kFullDiagSyncCycles = kSh4Clock / 20u;     // 50 ms guest time
constexpr std::uint64_t kSubmitTraceWindowCycles = kSh4Clock * 3u;
constexpr std::uint64_t kSubmitTraceSampleCycles = kSh4Clock / 50u; // 20 ms
constexpr std::uint64_t kV42InactivityCycles = kSh4Clock;             // T403 ~1 s
constexpr std::array<std::uint8_t,4> kDcIp{192,168,167,2};
constexpr std::array<std::uint8_t,4> kPeerIp{192,168,167,1};
constexpr std::array<std::uint8_t,4> kDreamcastLiveDns{46,101,91,123};
constexpr std::array<std::uint8_t,4> kSecondaryDns{178,156,255,64};
constexpr std::uint16_t kChuChuUdpPort = 9789u;

static constexpr std::array<std::uint8_t,0x21> kWriteMask = {
    0xFF,0xE0,0xEF,0xF3,0x95,0x3A,0x7F,0xEB,0xFF,0xFF,0x08,0x03,0x01,0x00,0x20,0x03,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xF7,0x00,0xFF,0xBF,0x24,0x1D,0xFF
};
static constexpr std::array<std::uint16_t,0x21> kIntMaskAddr = {
    0,0x247,0,0,0,0,0,0,0,0,0x246,0x245,0x244,0x243,0x242,0x241,
    0,0,0x089,0,0x38A,0,0x370,0x371,0,0,0x27D,0x27C,0,0,0,0,0
};
static constexpr std::array<unsigned,4> kRxFifoTrigger{1u,4u,8u,12u};

enum class ModemState : std::uint8_t { Invalid, Reset, Resetting, ControllerTest, DspTest, DspEnd, Normal };
enum class ConnectState : std::uint8_t { Disconnected, Dialing, Ringing, Handshaking, PreConnected, Connected, NegoComplete };
enum class V42Phase : std::uint8_t { None, Detection, Establish, Connected, Release };

#ifdef _WIN32
bool net_init() { static bool once=false, ok=false; if (!once) { WSADATA w{}; ok=WSAStartup(MAKEWORD(2,2),&w)==0; once=true; } return ok; }
void sock_close(DcrSocket s) { if (s!=kBadSocket) closesocket(s); }
bool sock_would_block() { const int e=WSAGetLastError(); return e==WSAEWOULDBLOCK || e==WSAEINPROGRESS || e==WSAEALREADY; }
bool sock_nonblocking(DcrSocket s) { u_long one=1; return ioctlsocket(s,FIONBIO,&one)==0; }
void sock_shutdown_send(DcrSocket s) { if (s!=kBadSocket) shutdown(s,SD_SEND); }
#else
bool net_init() { return true; }
void sock_close(DcrSocket s) { if (s!=kBadSocket) ::close(s); }
bool sock_would_block() { return errno==EWOULDBLOCK || errno==EAGAIN || errno==EINPROGRESS || errno==EALREADY; }
bool sock_nonblocking(DcrSocket s) { int f=fcntl(s,F_GETFL,0); return f>=0 && fcntl(s,F_SETFL,f|O_NONBLOCK)==0; }
void sock_shutdown_send(DcrSocket s) { if (s!=kBadSocket) shutdown(s,SHUT_WR); }
#endif

std::uint16_t ppp_fcs16(const std::uint8_t* data, std::size_t size) {
    std::uint16_t fcs=0xFFFFu;
    for (std::size_t i=0;i<size;++i) { fcs^=data[i]; for (unsigned b=0;b<8;++b) fcs=static_cast<std::uint16_t>((fcs&1u)?((fcs>>1u)^0x8408u):(fcs>>1u)); }
    return static_cast<std::uint16_t>(~fcs);
}

std::uint32_t sum16(const std::uint8_t* data, std::size_t n, std::uint32_t sum=0u) {
    while (n>=2u) { sum += static_cast<std::uint32_t>(data[0]<<8u) | data[1]; data+=2; n-=2; }
    if (n) sum += static_cast<std::uint32_t>(data[0])<<8u;
    return sum;
}
std::uint16_t finish_sum(std::uint32_t sum) { while (sum>>16u) sum=(sum&0xFFFFu)+(sum>>16u); return static_cast<std::uint16_t>(~sum); }
std::uint16_t ipv4_checksum(const std::uint8_t* data, std::size_t n) { return finish_sum(sum16(data,n)); }
std::uint16_t transport_checksum(const std::array<std::uint8_t,4>& src,const std::array<std::uint8_t,4>& dst,std::uint8_t proto,const std::uint8_t* data,std::size_t n) {
    std::uint32_t sum=0; sum=sum16(src.data(),4,sum); sum=sum16(dst.data(),4,sum); std::uint8_t ph[4]{0,proto,static_cast<std::uint8_t>(n>>8u),static_cast<std::uint8_t>(n)}; sum=sum16(ph,4,sum); sum=sum16(data,n,sum); return finish_sum(sum);
}

struct UdpEndpoint { DcrSocket sock{kBadSocket}; std::uint16_t guest_port{}; std::array<std::uint8_t,4> last_remote{}; std::uint16_t last_remote_port{}; bool chuchu{}; };
struct TcpKey { std::uint16_t sport{}, dport{}; std::uint32_t dip{}; bool operator==(const TcpKey& o) const noexcept { return sport==o.sport&&dport==o.dport&&dip==o.dip; } };
struct TcpKeyHash { std::size_t operator()(const TcpKey& k) const noexcept { return (static_cast<std::size_t>(k.dip)*1315423911u) ^ (static_cast<std::size_t>(k.sport)<<16u) ^ k.dport; } };
struct TcpFlow {
    DcrSocket sock{kBadSocket};
    bool connecting{}, connected{}, host_fin{}, guest_fin{};
    std::uint16_t sport{}, dport{};
    std::array<std::uint8_t,4> remote{};
    std::uint32_t guest_next{}, proxy_next{}, proxy_isn{}, last_guest_ack{};
    std::deque<std::uint8_t> pending_tx;
    std::uint64_t guest_payload_bytes{}, host_tx_bytes{}, host_rx_bytes{};
    unsigned guest_segment_logs{}, host_tx_logs{}, host_rx_logs{}, guest_rx_logs{}, seq_gap_logs{}, duplicate_logs{};
};

struct NetState {
    std::array<std::uint8_t,0x21> regs{};
    std::array<std::uint8_t,0x1000> dsp{};
    std::deque<std::uint8_t> rx_fifo;
    std::deque<std::uint8_t> backend_serial;
    std::vector<std::uint8_t> ppp_frame;
    bool ppp_in_frame{}, ppp_escape{}, lcp_request_sent{}, ipcp_request_sent{}, ppp_started{};
    std::uint8_t ppp_id{1u};
    // V.42/LAPM transport used by the Dreamcast modem when ASYN=0.
    V42Phase v42_phase{V42Phase::None};
    int v14_rx_state{}; // 0=idle, 1=data, 2=stop
    std::uint8_t v14_rx_byte{}; unsigned v14_rx_bits{};
    std::uint8_t v42_last_odp{}; unsigned v42_odp_count{};
    std::vector<std::uint8_t> v42_raw_bits;
    std::uint8_t v42_flag_shift{}; unsigned v42_flag_bits{}; bool v42_in_frame{};
    std::uint8_t v42_tx_byte{}; unsigned v42_tx_bits{};
    std::deque<std::uint8_t> v42_ppp_to_guest;
    std::uint8_t v42_tx_seq{}, v42_rx_seq{}, v42_tx_ack{};
    bool v42_receiver_ready{true};
    std::uint16_t v42_tx_max_size{128u};
    std::uint8_t v42_tx_window{15u};
    std::array<std::vector<std::uint8_t>,128> v42_sent_frames{};
    std::array<bool,128> v42_sent_valid{};
    std::vector<std::uint8_t> v42_probe;
    std::uint64_t v42_frames_in{}, v42_frames_out{};
    std::uint64_t v42_crc_errors{}, v42_abort_frames{}, v42_rr_frames{}, v42_rnr_frames{}, v42_rej_frames{}, v42_retransmits{}, v42_acked_frames{};
    std::uint64_t v42_last_activity_cycle{}, v42_last_keepalive_cycle{};
    std::uint64_t v42_idle_mark_bytes{}; bool v42_idle_mark_announced{};
    // Detection bookkeeping stays plugin-private: it must never change DCRuntime ABI.
    bool v42_adp_sent{}; unsigned v42_adp_groups{};
    ModemState state{ModemState::Invalid};
    ConnectState connect{ConnectState::Disconnected};
    std::uint64_t deadline{}, last_dial_cycle{}, next_serial_cycle{};
    float tx_fifo_size{}; float tx_rx_ratio{1.0f}; std::uint32_t cycles_per_byte{static_cast<std::uint32_t>(kSh4Clock*8u/14400u)};
    bool word_dsp_write{}, module_download{}, v8bis{};
    std::uint8_t reg1b_saved{}, download_crc{};
    std::uint64_t trace_count{};
    std::unordered_map<std::uint16_t,UdpEndpoint> udp;
    std::unordered_map<TcpKey,TcpFlow,TcpKeyHash> tcp;
    std::uint32_t ipv4_length_diag_logs=0;
    std::uint16_t ip_ident{1u};
    bool flash_repaired{};

    // cpp34r: all hot-path scheduling/diagnostics stay plugin-private so ABI1 is unchanged.
    std::uint64_t next_host_poll_cycle{}, next_full_diag_cycle{};
    std::uint64_t mmio_fast_reads{}, tick_fast_returns{}, socket_poll_batches{}, full_diag_syncs{};
    std::uint16_t last_maple_buttons{};
    std::uint64_t last_maple_changes{};
    bool submit_trace_active{};
    std::uint64_t submit_trace_until{}, submit_trace_next_sample{};
    std::uint32_t submit_trace_last_pc{};
    unsigned submit_trace_samples{}, submit_trace_tx_logs{};
    std::uint64_t trace_start_modem_tx{}, trace_start_ppp_tx{}, trace_start_ipv4{}, trace_start_tcp{};
    std::uint64_t trace_start_flash_program{}, trace_start_vmu_writes{};
    std::uint64_t trace_last_modem_tx{}, trace_last_ppp_tx{}, trace_last_ipv4{}, trace_last_tcp{};
    std::uint64_t trace_last_flash_program{}, trace_last_vmu_writes{};
};

std::mutex g_states_mutex;
std::unordered_map<DCRuntime*,std::unique_ptr<NetState>> g_states;
thread_local DCRuntime* g_tls_runtime = nullptr;
thread_local NetState* g_tls_state = nullptr;

NetState* find_state(DCRuntime& r) {
    if (g_tls_runtime==&r && g_tls_state) return g_tls_state;
    std::lock_guard<std::mutex> l(g_states_mutex);
    auto it=g_states.find(&r);
    if (it==g_states.end()) return nullptr;
    g_tls_runtime=&r; g_tls_state=it->second.get();
    return g_tls_state;
}
NetState& state_for(DCRuntime& r) {
    if (auto* s=find_state(r)) return *s;
    std::lock_guard<std::mutex> l(g_states_mutex);
    auto& p=g_states[&r]; if(!p)p=std::make_unique<NetState>();
    g_tls_runtime=&r; g_tls_state=p.get();
    return *p;
}

void close_network(NetState& s) {
    for (auto& [_,u]:s.udp) sock_close(u.sock); s.udp.clear();
    for (auto& [_,t]:s.tcp) sock_close(t.sock); s.tcp.clear();
}

void mirror_diag_fast(DCRuntime& r, const NetState& s) {
    r.modem_state=static_cast<std::uint32_t>(s.state);
    r.modem_connect_state=static_cast<std::uint32_t>(s.connect);
    r.modem_connect_due_cycle=s.deadline;
}
void mirror_diag_full(DCRuntime& r, NetState& s) {
    r.modem_regs=s.regs; r.modem_dspram=s.dsp; r.modem_rx_fifo=s.rx_fifo;
    mirror_diag_fast(r,s);
    ++s.full_diag_syncs;
}

std::uint8_t masked_status(const NetState& s,std::size_t reg) { const auto a=kIntMaskAddr[reg]; return a<s.dsp.size()?static_cast<std::uint8_t>(s.dsp[a]&s.regs[reg]):0u; }
void set_status(NetState& s,std::size_t reg,std::uint8_t bit,bool on) {
    const bool old=(s.regs[reg]&bit)!=0; if(old==on)return;
    const auto before=masked_status(s,reg); if(on)s.regs[reg]|=bit; else s.regs[reg]&=static_cast<std::uint8_t>(~bit);
    if ((s.regs[0x1F]&0x10u) && before!=masked_status(s,reg)) s.regs[0x1F]|=0x80u; // NSIE -> NSIA
}
void update_irq(DCRuntime& r,NetState& s) {
    if ((s.regs[0x1E]&0x04u)&&(s.regs[0x1E]&0x01u)) s.regs[0x1E]|=0x40u; else s.regs[0x1E]&=static_cast<std::uint8_t>(~0x40u);
    if ((s.regs[0x1E]&0x20u)&&(s.regs[0x1E]&0x08u)) s.regs[0x1E]|=0x80u; else s.regs[0x1E]&=static_cast<std::uint8_t>(~0x80u);
    const bool active=(s.regs[0x1F]&0xC0u)||(s.regs[0x1E]&0xC0u);
    if(active)r.holly_pending[kModemHollyBank]|=kModemHollyBit; else r.holly_pending[kModemHollyBank]&=~kModemHollyBit;
}
void refresh_fifo(DCRuntime& r,NetState& s) {
    if(!s.rx_fifo.empty()) { s.regs[0]=s.rx_fifo.front(); const unsigned trig=kRxFifoTrigger[(s.dsp[0x32C]>>6u)&3u]; if(s.rx_fifo.size()>=trig)s.regs[0x1E]|=1u; else s.regs[0x1E]&=0xFEu; if(s.regs[4]&0x10u)set_status(s,0x0C,0x02u,true); set_status(s,1,0x02u,s.rx_fifo.size()>=8u); }
    else { s.regs[0x1E]&=0xFEu; if(s.regs[4]&0x10u)set_status(s,0x0C,0x02u,false); set_status(s,1,0x02u,false); }
    if(s.regs[4]&0x10u || s.module_download) { set_status(s,0x0D,0x02u,s.tx_fifo_size<=15.f); set_status(s,1,0x04u,s.tx_fifo_size>=8.f); }
    else { set_status(s,0x0D,0x02u,s.tx_fifo_size==0.f); set_status(s,1,0x04u,s.tx_fifo_size>=1.f); }
    update_irq(r,s);
}

void init_sparse_dsp(NetState& s) {
    s.dsp.fill(0);
    s.dsp[0x09F]=0x97; s.dsp[0x0B3]=0x01; s.dsp[0x133]=0x30;
    const std::array<std::uint8_t,16> a218{0xDD,0xAD,0x08,0x07,0x99,0x00,0x23,0xA0,0,0,0,0,0x56,0,0,0}; std::copy(a218.begin(),a218.end(),s.dsp.begin()+0x218);
    s.dsp[0x209]=0x20; s.dsp[0x2DA]=0x01; s.dsp[0x2E4]=0x0D; s.dsp[0x2E5]=0x0E;
    const std::array<std::uint8_t,16> a300{0,0,0,0,0xC0,0x28,0x40,0x03,0x84,0xCC,0,0x76,0x84,0xB1,0x14,0x02}; std::copy(a300.begin(),a300.end(),s.dsp.begin()+0x300);
    s.dsp[0x310]=0x08; s.dsp[0x311]=0x08; s.dsp[0x312]=0xA4; s.dsp[0x313]=0xA4; s.dsp[0x314]=0xA0; s.dsp[0x32C]=0xCB;
    const std::array<std::uint8_t,16> a3b0{0x20,0x1A,0x12,0x0C,0x0A,0x09,0x05,0x03,0x02,0x00,0x90,0x28,0x10,0x04,0x01,0x00}; std::copy(a3b0.begin(),a3b0.end(),s.dsp.begin()+0x3B0);
    s.dsp[0x3DB]=0x40; s.dsp[0x409]=0xCE;
}

void normal_defaults(DCRuntime& r,NetState& s) {
    s.regs.fill(0); init_sparse_dsp(s); s.rx_fifo.clear(); s.backend_serial.clear(); s.ppp_frame.clear(); s.ppp_in_frame=s.ppp_escape=false; s.lcp_request_sent=s.ipcp_request_sent=false; s.ppp_started=false; s.v42_phase=V42Phase::None; s.v14_rx_state=0; s.v14_rx_byte=0; s.v14_rx_bits=0; s.v42_last_odp=0; s.v42_odp_count=0; s.v42_raw_bits.clear(); s.v42_flag_shift=0; s.v42_flag_bits=0; s.v42_in_frame=false; s.v42_tx_byte=0; s.v42_tx_bits=0; s.v42_ppp_to_guest.clear(); s.v42_tx_seq=s.v42_rx_seq=s.v42_tx_ack=0; s.v42_receiver_ready=true; s.v42_tx_max_size=128u; s.v42_tx_window=15u; s.v42_sent_valid.fill(false); for(auto& f:s.v42_sent_frames) f.clear(); s.v42_probe.clear(); s.v42_frames_in=s.v42_frames_out=0; s.v42_crc_errors=s.v42_abort_frames=s.v42_rr_frames=s.v42_rnr_frames=s.v42_rej_frames=s.v42_retransmits=s.v42_acked_frames=0; s.v42_last_activity_cycle=s.v42_last_keepalive_cycle=0; s.v42_idle_mark_bytes=0; s.v42_idle_mark_announced=false; s.v42_adp_sent=false; s.v42_adp_groups=0;
    s.regs[0x05]=0x08; s.regs[0x12]=0x76; s.regs[0x09]=0x24; s.regs[0x15]=0x30; s.regs[0x07]=0x80; s.regs[0x03]=0x10; s.regs[0x02]=0x40; s.regs[0x13]=0x09; s.regs[0x1E]=0x08;
    s.connect=ConnectState::Disconnected; s.deadline=0; s.last_dial_cycle=0; s.tx_fifo_size=0; s.v8bis=false; s.module_download=false; s.word_dsp_write=false; close_network(s); refresh_fifo(r,s); mirror_diag_full(r,s);
    std::cout << "[MODEM] Rockwell 33.6K normal state | Flycast-compatible register/DSP/FIFO model\n";
}

void set_reg16(NetState& s,std::size_t hi,std::size_t lo,std::uint16_t v){s.regs[hi]=static_cast<std::uint8_t>(v>>8u);s.regs[lo]=static_cast<std::uint8_t>(v);}
void controller_test_start(DCRuntime& r,NetState& s) {
    s.state=ModemState::ControllerTest; s.regs[0x1E]|=0x08u; set_reg16(s,0x1D,0x1C,0xEA3C); set_reg16(s,0x1B,0x1A,0x5536); set_reg16(s,0x19,0x18,0x5F4C); set_reg16(s,0x17,0x16,0x3835); set_reg16(s,0x15,0x14,0x0801); set_reg16(s,0x13,0x12,0x3730); set_reg16(s,0x11,0x00,0x4241); s.state=ModemState::DspTest; s.deadline=r.pvr_spg_sh4_cycles+kSh4Clock/20u; mirror_diag_full(r,s);
}
void dsp_test_start(DCRuntime& r,NetState& s) {
    s.state=ModemState::DspEnd; s.regs[0x1E]|=0x08u; set_reg16(s,0x1B,0x1A,0xF083); set_reg16(s,0x19,0x18,0x46EE); set_reg16(s,0x17,0x16,0x00FA); set_reg16(s,0x15,0x14,0x0A09); set_reg16(s,0x13,0x12,0x3730); set_reg16(s,0x11,0x00,0x2041); s.deadline=r.pvr_spg_sh4_cycles+kSh4Clock/20u; mirror_diag_full(r,s);
}
void hard_reset(DCRuntime& r,NetState& s,std::uint8_t v) {
    if(v==0){close_network(s);s.regs.fill(0);s.rx_fifo.clear();s.backend_serial.clear();s.state=ModemState::Reset;s.connect=ConnectState::Disconnected;s.deadline=0;s.ppp_started=false;std::cout<<"[MODEM] reset asserted\n";}
    else if(s.state==ModemState::Reset){s.regs.fill(0);s.state=ModemState::Resetting;controller_test_start(r,s);s.regs[0x20]=v;std::cout<<"[MODEM] reset released | controller/DSP self-test\n";}
    else s.regs[0x20]=v;
    mirror_diag_full(r,s);
}

bool configure_speed(NetState& s) {
    static constexpr int v90[]{0,28800,29333,30667,32000,33333,34667,36000,37333,38667,40000,41333,42667,44000,45333,46667,48000,49333,50667,52000,53333,54667,56000};
    static constexpr int v34[]{0,2400,4800,7200,9600,12000,14400,16800,19200,21600,24000,26400,28800,31200,33600};
    int speed=0,rx=0; s.regs[0x0E]&=0xE0u; const auto c=s.regs[0x12];
    if(c>=0xE0&&c<=0xF6){rx=v90[c-0xE0]; if(rx){speed=33600;s.regs[0x0E]|=0x10;s.dsp[0x2E4]=c-0xE0;s.dsp[0x2E5]=0x0D;}}
    else if(c>=0x90&&c<=0x9D){if(c!=0x90){rx=30000+(c-0x90)*2000;speed=33600;s.regs[0x0E]|=0x10;}}
    else if(c>=0xC0&&c<=0xCE){speed=v34[c-0xC0];if(speed)s.regs[0x0E]|=static_cast<std::uint8_t>(c-0xC0+2);s.dsp[0x2E4]=s.dsp[0x2E5]=c-0xC0;}
    else { switch(c){case 0x31:speed=14400;break;case 0x32:speed=12000;break;case 0x34:speed=9600;break;case 0x38:speed=7200;break;case 0x76:speed=14400;break;case 0x72:speed=12000;break;case 0x74:case 0x75:speed=9600;break;case 0x78:speed=7200;break;case 0x71:speed=4800;break;case 0xB1:speed=14400;break;case 0xB2:speed=12000;break;case 0xB4:speed=9600;break;case 0xB8:speed=7200;break;case 0x14:speed=9600;break;case 0x12:speed=7200;break;case 0x11:speed=4800;break;case 0x02:case 0x23:speed=4800;break;case 0x01:case 0x84:speed=2400;break;case 0x82:case 0x52:case 0x62:speed=1200;break;case 0x51:speed=600;break;case 0xA0:case 0xA8:case 0x60:speed=300;break;case 0xA4:speed=1200;rx=75;break;case 0xA1:speed=75;rx=1200;break;default:break;} }
    if(!rx)rx=speed; if(!speed){s.cycles_per_byte=static_cast<std::uint32_t>(kSh4Clock*8u/300u);s.tx_rx_ratio=1.f;return false;}
    if((s.regs[0x0E]&0x1Fu)==0){switch(speed){case 14400:s.regs[0x0E]|=7;break;case 12000:s.regs[0x0E]|=6;break;case 9600:s.regs[0x0E]|=5;break;case 7200:s.regs[0x0E]|=8;break;case 4800:s.regs[0x0E]|=4;break;case 2400:s.regs[0x0E]|=3;break;case 1200:s.regs[0x0E]|=2;break;case 600:s.regs[0x0E]|=1;break;default:break;}}
    s.cycles_per_byte=static_cast<std::uint32_t>(kSh4Clock*8ull/static_cast<unsigned>(speed)); s.tx_rx_ratio=static_cast<float>(speed)/static_cast<float>(rx); std::cout<<"[MODEM] negotiated "<<rx<<"/"<<speed<<" bps\n"; return true;
}

void ppp_queue_bytes(DCRuntime& r,NetState& s,const std::vector<std::uint8_t>& b){
    // ASYN=0 means the modem presents a V.42/LAPM synchronous link to the host.
    // Queue PPP bytes for LAPM wrapping instead of exposing them directly on the modem wire.
    if (s.connect==ConnectState::Connected && (s.regs[8]&0x80u)==0) {
        for (auto v:b) s.v42_ppp_to_guest.push_back(v);
    } else {
        for (auto v:b) s.backend_serial.push_back(v);
    }
    r.modem_rx_bytes+=b.size();
}
void ppp_send(NetState& s,DCRuntime& r,std::uint16_t proto,const std::vector<std::uint8_t>& payload){std::vector<std::uint8_t> raw{0xFF,0x03,static_cast<std::uint8_t>(proto>>8u),static_cast<std::uint8_t>(proto)};raw.insert(raw.end(),payload.begin(),payload.end());auto f=ppp_fcs16(raw.data(),raw.size());raw.push_back(static_cast<std::uint8_t>(f));raw.push_back(static_cast<std::uint8_t>(f>>8u));std::vector<std::uint8_t> out;out.push_back(0x7E);for(auto v:raw){if(v==0x7E||v==0x7D||v<0x20){out.push_back(0x7D);out.push_back(v^0x20);}else out.push_back(v);}out.push_back(0x7E);++r.modem_ppp_rx_frames;ppp_queue_bytes(r,s,out);}
void ppp_control(NetState& s,DCRuntime& r,std::uint16_t proto,std::uint8_t code,std::uint8_t id,const std::vector<std::uint8_t>& data){std::vector<std::uint8_t> p{code,id,static_cast<std::uint8_t>((data.size()+4)>>8u),static_cast<std::uint8_t>(data.size()+4)};p.insert(p.end(),data.begin(),data.end());ppp_send(s,r,proto,p);}

std::vector<std::uint8_t> ipv4_packet(NetState& s,const std::array<std::uint8_t,4>& src,const std::array<std::uint8_t,4>& dst,std::uint8_t proto,const std::vector<std::uint8_t>& l4){std::vector<std::uint8_t> p(20u+l4.size(),0);p[0]=0x45;put_be16(p.data()+2,static_cast<std::uint16_t>(p.size()));put_be16(p.data()+4,s.ip_ident++);p[6]=0x40;p[8]=64;p[9]=proto;std::copy(src.begin(),src.end(),p.begin()+12);std::copy(dst.begin(),dst.end(),p.begin()+16);put_be16(p.data()+10,ipv4_checksum(p.data(),20));std::copy(l4.begin(),l4.end(),p.begin()+20);return p;}
void send_udp_to_guest(NetState& s,DCRuntime& r,const std::array<std::uint8_t,4>& src,std::uint16_t sport,std::uint16_t dport,const std::uint8_t* data,std::size_t n){std::vector<std::uint8_t> u(8+n,0);put_be16(u.data(),sport);put_be16(u.data()+2,dport);put_be16(u.data()+4,static_cast<std::uint16_t>(u.size()));std::memcpy(u.data()+8,data,n);auto ip=ipv4_packet(s,src,kDcIp,17,u);ppp_send(s,r,0x0021,ip);}
std::string ip_text(const std::array<std::uint8_t,4>& ip){
    std::ostringstream o;o<<static_cast<unsigned>(ip[0])<<'.'<<static_cast<unsigned>(ip[1])<<'.'<<static_cast<unsigned>(ip[2])<<'.'<<static_cast<unsigned>(ip[3]);return o.str();
}
std::string tcp_flags_text(std::uint8_t f){
    std::string x;if(f&0x02)x+="S";if(f&0x10)x+="A";if(f&0x08)x+="P";if(f&0x01)x+="F";if(f&0x04)x+="R";if(x.empty())x="-";return x;
}
std::string bytes_preview(const std::uint8_t* data,std::size_t n){
    const std::size_t lim=std::min<std::size_t>(n,64);std::ostringstream o;o<<"hex="<<std::hex<<std::setfill('0');
    for(std::size_t i=0;i<lim;++i){if(i)o<<' ';o<<std::setw(2)<<static_cast<unsigned>(data[i]);}
    if(n>lim)o<<" ...";o<<" | ascii="<<std::dec;
    for(std::size_t i=0;i<lim;++i){const auto c=data[i];o<<((c>=32&&c<=126)?static_cast<char>(c):'.');}
    if(n>lim)o<<"...";return o.str();
}
#ifdef _WIN32
int sock_error_code(){return WSAGetLastError();}
#else
int sock_error_code(){return errno;}
#endif
inline bool tcp_seq_before(std::uint32_t a,std::uint32_t b){return static_cast<std::int32_t>(a-b)<0;}

void send_tcp_to_guest(NetState& s,DCRuntime& r,TcpFlow& f,std::uint8_t flags,const std::uint8_t* data=nullptr,std::size_t n=0){
    const auto seq=f.proxy_next;
    std::vector<std::uint8_t> t(20+n,0);put_be16(t.data(),f.dport);put_be16(t.data()+2,f.sport);put_be32(t.data()+4,seq);put_be32(t.data()+8,f.guest_next);t[12]=0x50;t[13]=flags;put_be16(t.data()+14,0xFFFF);if(n)std::memcpy(t.data()+20,data,n);auto c=transport_checksum(f.remote,kDcIp,6,t.data(),t.size());put_be16(t.data()+16,c);auto ip=ipv4_packet(s,f.remote,kDcIp,6,t);ppp_send(s,r,0x0021,ip);
    if((n||flags&0x07u)&&f.guest_rx_logs<24){++f.guest_rx_logs;std::cout<<"[TCP DC-RX] "<<ip_text(f.remote)<<':'<<f.dport<<" -> "<<f.sport<<" flags="<<tcp_flags_text(flags)<<" seq="<<seq<<" ack="<<f.guest_next<<" bytes="<<n;if(n)std::cout<<" | "<<bytes_preview(data,n);std::cout<<"\n";}
    if(flags&0x02u)++f.proxy_next;if(flags&0x01u)++f.proxy_next;if(n)f.proxy_next+=static_cast<std::uint32_t>(n);
}

UdpEndpoint& udp_endpoint(NetState& s,std::uint16_t guest_port){auto it=s.udp.find(guest_port);if(it!=s.udp.end())return it->second;UdpEndpoint u{};u.guest_port=guest_port;u.chuchu=guest_port==kChuChuUdpPort;u.sock=socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);if(u.sock!=kBadSocket){sock_nonblocking(u.sock);sockaddr_in a{};a.sin_family=AF_INET;a.sin_addr.s_addr=htonl(INADDR_ANY);a.sin_port=htons(guest_port);if(bind(u.sock,reinterpret_cast<sockaddr*>(&a),sizeof(a))!=0){a.sin_port=0;bind(u.sock,reinterpret_cast<sockaddr*>(&a),sizeof(a));}}auto [ni,_]=s.udp.emplace(guest_port,std::move(u));return ni->second;}
void udp_out(NetState& s,DCRuntime& r,const std::vector<std::uint8_t>& ip){if(ip.size()<28||ip[9]!=17)return;const auto ihl=(ip[0]&0x0F)*4u;if(ihl<20||ip.size()<ihl+8)return;const auto sp=be16(ip.data()+ihl),dp=be16(ip.data()+ihl+2),ulen=be16(ip.data()+ihl+4);if(ulen<8||ihl+ulen>ip.size())return;auto& u=udp_endpoint(s,sp);if(u.sock==kBadSocket)return;std::copy_n(ip.data()+16,4,u.last_remote.begin());u.last_remote_port=dp;sockaddr_in peer{};peer.sin_family=AF_INET;peer.sin_port=htons(dp);std::memcpy(&peer.sin_addr.s_addr,ip.data()+16,4);sendto(u.sock,reinterpret_cast<const char*>(ip.data()+ihl+8),static_cast<int>(ulen-8),0,reinterpret_cast<sockaddr*>(&peer),sizeof(peer));if(dp==53)++r.modem_dns_frames;}

void tcp_close(TcpFlow& f){sock_close(f.sock);f.sock=kBadSocket;f.connecting=f.connected=false;}
void tcp_open(NetState& s,DCRuntime& r,const TcpKey& key,const std::vector<std::uint8_t>& ip,std::size_t off){
    TcpFlow f{};f.sport=key.sport;f.dport=key.dport;std::copy_n(ip.data()+16,4,f.remote.begin());f.guest_next=be32(ip.data()+off+4)+1u;f.proxy_isn=0x4D430000u ^ (static_cast<std::uint32_t>(f.sport)<<8u) ^ f.dport ^ key.dip;f.proxy_next=f.proxy_isn;
    std::cout<<"[TCP OPEN] DC "<<static_cast<unsigned>(kDcIp[0])<<'.'<<static_cast<unsigned>(kDcIp[1])<<'.'<<static_cast<unsigned>(kDcIp[2])<<'.'<<static_cast<unsigned>(kDcIp[3])<<':'<<f.sport<<" -> "<<ip_text(f.remote)<<':'<<f.dport<<" | guest_next="<<f.guest_next<<" proxy_isn="<<f.proxy_isn<<"\n";
    f.sock=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);if(f.sock==kBadSocket){std::cout<<"[TCP ERROR] socket() failed err="<<sock_error_code()<<"\n";return;}sock_nonblocking(f.sock);int one=1;setsockopt(f.sock,IPPROTO_TCP,TCP_NODELAY,reinterpret_cast<const char*>(&one),sizeof(one));sockaddr_in peer{};peer.sin_family=AF_INET;peer.sin_port=htons(f.dport);std::memcpy(&peer.sin_addr.s_addr,ip.data()+16,4);const int rc=connect(f.sock,reinterpret_cast<sockaddr*>(&peer),sizeof(peer));if(rc==0)f.connected=true;else if(sock_would_block())f.connecting=true;else{std::cout<<"[TCP ERROR] connect "<<ip_text(f.remote)<<':'<<f.dport<<" err="<<sock_error_code()<<"\n";tcp_close(f);return;}auto [it,_]=s.tcp.emplace(key,std::move(f));if(it->second.connected){send_tcp_to_guest(s,r,it->second,0x12);std::cout<<"[TCP] connected immediately "<<it->second.sport<<" -> "<<ip_text(it->second.remote)<<':'<<it->second.dport<<"\n";}
}
void tcp_out(NetState& s,DCRuntime& r,const std::vector<std::uint8_t>& ip){
    if(ip.size()<40||ip[9]!=6)return;const std::size_t ihl=(ip[0]&0x0F)*4u;if(ihl<20||ip.size()<ihl+20)return;const std::size_t ipv4_total=be16(ip.data()+2);if(ipv4_total<ihl+20||ipv4_total>ip.size())return;const std::size_t off=ihl;const auto sp=be16(ip.data()+off),dp=be16(ip.data()+off+2);std::uint32_t dip=be32(ip.data()+16);TcpKey key{sp,dp,dip};const auto flags=ip[off+13];const auto seq=be32(ip.data()+off+4);const auto ack=be32(ip.data()+off+8);const std::size_t thl=((ip[off+12]>>4u)&0x0Fu)*4u;if(thl<20||ipv4_total<off+thl)return;const std::size_t n=ipv4_total-(off+thl);auto it=s.tcp.find(key);
    if((flags&0x02u)&&it==s.tcp.end()){tcp_open(s,r,key,ip,off);return;}if(it==s.tcp.end())return;auto& f=it->second;f.last_guest_ack=ack;
    if((n||flags&0x07u)&&f.guest_segment_logs<32){++f.guest_segment_logs;std::cout<<"[TCP DC-TX] "<<f.sport<<" -> "<<ip_text(f.remote)<<':'<<f.dport<<" flags="<<tcp_flags_text(flags)<<" seq="<<seq<<" ack="<<ack<<" expected="<<f.guest_next<<" bytes="<<n;if(n)std::cout<<" | "<<bytes_preview(ip.data()+off+thl,n);std::cout<<"\n";}
    if(flags&0x04u){std::cout<<"[TCP] guest RST "<<f.sport<<" -> "<<ip_text(f.remote)<<':'<<f.dport<<"\n";tcp_close(f);s.tcp.erase(it);return;}
    if(n&&f.connected){
        const auto* d=ip.data()+off+thl;std::size_t skip=0,new_bytes=0;
        if(seq==f.guest_next){new_bytes=n;}
        else if(tcp_seq_before(seq,f.guest_next)){
            const std::uint32_t already=f.guest_next-seq;
            if(already<n){skip=static_cast<std::size_t>(already);new_bytes=n-skip;if(f.duplicate_logs<8){++f.duplicate_logs;std::cout<<"[TCP SEQ] overlap trimmed="<<skip<<" new="<<new_bytes<<" expected="<<f.guest_next<<"\n";}}
            else if(f.duplicate_logs<8){++f.duplicate_logs;std::cout<<"[TCP SEQ] duplicate ignored bytes="<<n<<" seq="<<seq<<" expected="<<f.guest_next<<"\n";}
        } else if(f.seq_gap_logs<8){++f.seq_gap_logs;std::cout<<"[TCP SEQ] GAP seq="<<seq<<" expected="<<f.guest_next<<" bytes="<<n<<" -> ACK expected only\n";}
        if(new_bytes){for(std::size_t i=0;i<new_bytes;++i)f.pending_tx.push_back(d[skip+i]);f.guest_next+=static_cast<std::uint32_t>(new_bytes);f.guest_payload_bytes+=new_bytes;}
        send_tcp_to_guest(s,r,f,0x10);
    }
    if(flags&0x01u){const auto fin_end=seq+static_cast<std::uint32_t>(n)+1u;if(seq+n==f.guest_next||tcp_seq_before(f.guest_next,fin_end))f.guest_next=fin_end;f.guest_fin=true;std::cout<<"[TCP] guest FIN "<<f.sport<<" -> "<<ip_text(f.remote)<<':'<<f.dport<<"\n";send_tcp_to_guest(s,r,f,0x10);sock_shutdown_send(f.sock);}
}
void poll_udp(NetState& s,DCRuntime& r){std::array<std::uint8_t,2048> buf{};for(auto& [port,u]:s.udp){if(u.sock==kBadSocket)continue;for(unsigned loops=0;loops<8;++loops){sockaddr_in from{};
#ifdef _WIN32
int flen=sizeof(from);
#else
socklen_t flen=sizeof(from);
#endif
int n=recvfrom(u.sock,reinterpret_cast<char*>(buf.data()),static_cast<int>(buf.size()),0,reinterpret_cast<sockaddr*>(&from),&flen);if(n<=0){if(n<0&&sock_would_block()){}break;}std::array<std::uint8_t,4> src{};std::memcpy(src.data(),&from.sin_addr.s_addr,4);send_udp_to_guest(s,r,src,ntohs(from.sin_port),port,buf.data(),static_cast<std::size_t>(n));}}
}
void poll_tcp(NetState& s,DCRuntime& r){
    std::array<std::uint8_t,536> buf{};for(auto it=s.tcp.begin();it!=s.tcp.end();){auto& f=it->second;bool erase=false;
    if(f.connecting){fd_set w,e;FD_ZERO(&w);FD_ZERO(&e);FD_SET(f.sock,&w);FD_SET(f.sock,&e);timeval tv{};const int rc=select(static_cast<int>(f.sock+1),nullptr,&w,&e,&tv);if(rc>0&&(FD_ISSET(f.sock,&w)||FD_ISSET(f.sock,&e))){int err=0;
#ifdef _WIN32
    int len=sizeof(err);
#else
    socklen_t len=sizeof(err);
#endif
    getsockopt(f.sock,SOL_SOCKET,SO_ERROR,reinterpret_cast<char*>(&err),&len);f.connecting=false;if(err==0){f.connected=true;send_tcp_to_guest(s,r,f,0x12);std::cout<<"[TCP] connected "<<f.sport<<" -> "<<ip_text(f.remote)<<':'<<f.dport<<"\n";}else{std::cout<<"[TCP ERROR] async connect "<<ip_text(f.remote)<<':'<<f.dport<<" err="<<err<<"\n";erase=true;}}}
    if(!erase&&f.connected){
        while(!f.pending_tx.empty()){
            std::array<std::uint8_t,1024> out{};std::size_t want=std::min<std::size_t>(out.size(),f.pending_tx.size());for(std::size_t i=0;i<want;++i)out[i]=f.pending_tx[i];int wr=send(f.sock,reinterpret_cast<const char*>(out.data()),static_cast<int>(want),0);
            if(wr>0){if(f.host_tx_logs<24){++f.host_tx_logs;std::cout<<"[TCP HOST-TX] "<<f.sport<<" -> "<<ip_text(f.remote)<<':'<<f.dport<<" bytes="<<wr<<" queued_after="<<(f.pending_tx.size()-static_cast<std::size_t>(wr))<<" | "<<bytes_preview(out.data(),static_cast<std::size_t>(wr))<<"\n";}f.host_tx_bytes+=static_cast<std::uint64_t>(wr);for(int i=0;i<wr;++i)f.pending_tx.pop_front();}
            else{if(wr<0&&sock_would_block())break;std::cout<<"[TCP ERROR] host send "<<ip_text(f.remote)<<':'<<f.dport<<" err="<<sock_error_code()<<"\n";erase=true;break;}
        }
        if(!erase){int n=recv(f.sock,reinterpret_cast<char*>(buf.data()),static_cast<int>(buf.size()),0);if(n>0){if(f.host_rx_logs<24){++f.host_rx_logs;std::cout<<"[TCP HOST-RX] "<<ip_text(f.remote)<<':'<<f.dport<<" -> "<<f.sport<<" bytes="<<n<<" | "<<bytes_preview(buf.data(),static_cast<std::size_t>(n))<<"\n";}f.host_rx_bytes+=static_cast<std::uint64_t>(n);send_tcp_to_guest(s,r,f,0x18,buf.data(),static_cast<std::size_t>(n));}else if(n==0&&!f.host_fin){f.host_fin=true;std::cout<<"[TCP HOST-FIN] "<<ip_text(f.remote)<<':'<<f.dport<<" totals tx="<<f.host_tx_bytes<<" rx="<<f.host_rx_bytes<<" guest_payload="<<f.guest_payload_bytes<<"\n";send_tcp_to_guest(s,r,f,0x11);}else if(n<0&&!sock_would_block()){std::cout<<"[TCP ERROR] host recv "<<ip_text(f.remote)<<':'<<f.dport<<" err="<<sock_error_code()<<" totals tx="<<f.host_tx_bytes<<" rx="<<f.host_rx_bytes<<"\n";erase=true;}}
    }
    if(erase){tcp_close(f);it=s.tcp.erase(it);}else ++it;}
}

void ppp_ipv4(NetState& s,DCRuntime& r,const std::vector<std::uint8_t>& wire){
    if(wire.size()<20||(wire[0]>>4u)!=4)return;const std::size_t ihl=(wire[0]&0x0Fu)*4u;if(ihl<20||wire.size()<ihl)return;const std::size_t total=be16(wire.data()+2);if(total<ihl||total>wire.size()){if(s.ipv4_length_diag_logs<8){++s.ipv4_length_diag_logs;std::cout<<"[IPV4 LENGTH] invalid total="<<total<<" wire="<<wire.size()<<" ihl="<<ihl<<" -> drop\n";}return;}if(total<wire.size()&&s.ipv4_length_diag_logs<8){++s.ipv4_length_diag_logs;std::cout<<"[IPV4 LENGTH] clamp wire="<<wire.size()<<" total="<<total<<" tail="<<(wire.size()-total)<<" proto="<<static_cast<unsigned>(wire[9])<<"\n";}std::vector<std::uint8_t> ip(wire.begin(),wire.begin()+static_cast<std::ptrdiff_t>(total));++r.modem_ipv4_frames;if(ip[9]==17){++r.modem_udp_frames;udp_out(s,r,ip);}else if(ip[9]==6){++r.modem_tcp_frames;tcp_out(s,r,ip);}}
void ppp_frame(NetState& s,DCRuntime& r,std::vector<std::uint8_t> frame){if(frame.size()<4)return;auto got=static_cast<std::uint16_t>(frame[frame.size()-2]|(frame.back()<<8u));frame.resize(frame.size()-2);if(got!=ppp_fcs16(frame.data(),frame.size()))return;std::size_t pos=0;if(frame.size()>=2&&frame[0]==0xFF&&frame[1]==0x03)pos=2;if(pos>=frame.size())return;std::uint16_t proto;if(frame[pos]&1u)proto=frame[pos++];else{if(pos+1>=frame.size())return;proto=static_cast<std::uint16_t>((frame[pos]<<8u)|frame[pos+1]);pos+=2;}std::vector<std::uint8_t> p(frame.begin()+static_cast<std::ptrdiff_t>(pos),frame.end());++r.modem_ppp_tx_frames;
if(proto==0xC021){++r.modem_ppp_lcp_frames;if(p.size()>=4){auto code=p[0],id=p[1];auto len=std::min<std::size_t>(p.size(),be16(p.data()+2));std::vector<std::uint8_t> opts(p.begin()+4,p.begin()+static_cast<std::ptrdiff_t>(len));if(code==1)ppp_control(s,r,proto,2,id,opts);else if(code==9)ppp_control(s,r,proto,10,id,opts);else if(code==5)ppp_control(s,r,proto,6,id,opts);if(!s.lcp_request_sent){s.lcp_request_sent=true;std::vector<std::uint8_t> ours{1,4,0x05,0xDC};ppp_control(s,r,proto,1,s.ppp_id++,ours);}}return;}
if(proto==0xC023){++r.modem_ppp_pap_frames;if(p.size()>=4&&p[0]==1){std::vector<std::uint8_t> msg{2,'O','K'};ppp_control(s,r,proto,2,p[1],msg);std::cout<<"[PPP] PAP accepted\n";}return;}
if(proto==0xC223){if(p.size()>=4){std::vector<std::uint8_t> msg{'O','K'};ppp_control(s,r,proto,3,p[1],msg);std::cout<<"[PPP] CHAP success compatibility response\n";}return;}
if(proto==0x8021){++r.modem_ppp_ipcp_frames;if(p.size()>=4&&p[0]==1){auto id=p[1];auto len=std::min<std::size_t>(p.size(),be16(p.data()+2));std::vector<std::uint8_t> nak;for(std::size_t i=4;i+2<=len;){auto t=p[i],l=p[i+1];if(l<2||i+l>len)break;if(l==6&&(t==3||t==129||t==131)){bool zero=true;for(int j=0;j<4;++j)zero&=p[i+2+j]==0;if(zero){nak.push_back(t);nak.push_back(6);const auto& v=t==3?kDcIp:(t==129?kDreamcastLiveDns:kSecondaryDns);nak.insert(nak.end(),v.begin(),v.end());}}i+=l;}if(!nak.empty())ppp_control(s,r,proto,3,id,nak);else{std::vector<std::uint8_t> exact(p.begin()+4,p.begin()+static_cast<std::ptrdiff_t>(len));ppp_control(s,r,proto,2,id,exact);}if(!s.ipcp_request_sent){s.ipcp_request_sent=true;std::vector<std::uint8_t> own{3,6,kPeerIp[0],kPeerIp[1],kPeerIp[2],kPeerIp[3]};ppp_control(s,r,proto,1,s.ppp_id++,own);}}if(r.modem_ppp_ipcp_frames<=8)std::cout<<"[PPP] IPCP | dc=192.168.167.2 peer=192.168.167.1 dns=46.101.91.123\n";return;}
if(proto==0x0021){ppp_ipv4(s,r,p);return;}
if(r.modem_ppp_tx_frames<10)std::cout<<"[PPP] protocol 0x"<<std::hex<<proto<<std::dec<<" observed\n";}
void ppp_serial_write(NetState& s,DCRuntime& r,std::uint8_t v){if(v==0x7E){if(s.ppp_in_frame&&!s.ppp_frame.empty())ppp_frame(s,r,s.ppp_frame);s.ppp_frame.clear();s.ppp_in_frame=true;s.ppp_escape=false;return;}if(!s.ppp_in_frame)return;if(v==0x7D){s.ppp_escape=true;return;}if(s.ppp_escape){v^=0x20;s.ppp_escape=false;}if(s.ppp_frame.size()<8192)s.ppp_frame.push_back(v);}



// --- Minimal V.42/LAPM bridge -------------------------------------------------
// ChuChu Rocket uses the modem in synchronous mode (ASYN=0). In that mode the
// bytes visible at TBUFFER/RBUFFER are a V.42 bitstream, not PPP octets. This
// implementation terminates the local LAPM session and exposes its information
// field to the existing PPP stack. It intentionally negotiates V.42bis off.

void v42_emit_bit(NetState& s, unsigned bit) {
    if (bit) s.v42_tx_byte |= static_cast<std::uint8_t>(1u << s.v42_tx_bits);
    if (++s.v42_tx_bits == 8u) {
        s.backend_serial.push_back(s.v42_tx_byte);
        s.v42_tx_byte=0; s.v42_tx_bits=0;
    }
}
void v42_emit_flag(NetState& s) {
    constexpr std::uint8_t f=0x7Eu;
    for (unsigned i=0;i<8u;++i) v42_emit_bit(s,(f>>i)&1u);
}
void v42_emit_data_byte(NetState& s,std::uint8_t v,unsigned& ones) {
    for (unsigned i=0;i<8u;++i) {
        const unsigned bit=(v>>i)&1u;
        v42_emit_bit(s,bit);
        if (bit) {
            if (++ones==5u) { v42_emit_bit(s,0); ones=0; }
        } else ones=0;
    }
}
void v42_send_hdlc(NetState& s,const std::vector<std::uint8_t>& data) {
    if (data.empty()) return;
    v42_emit_flag(s);
    unsigned ones=0;
    for (auto b:data) v42_emit_data_byte(s,b,ones);
    const auto f=ppp_fcs16(data.data(),data.size());
    v42_emit_data_byte(s,static_cast<std::uint8_t>(f),ones);
    v42_emit_data_byte(s,static_cast<std::uint8_t>(f>>8u),ones);
    // Two flags ensure at least one complete closing flag reaches the byte stream
    // even when bit stuffing leaves the channel non-byte-aligned.
    v42_emit_flag(s); v42_emit_flag(s);
    ++s.v42_frames_out;
}
void v42_emit_v14_char(NetState& s,std::uint8_t c,unsigned stop_bits) {
    v42_emit_bit(s,0); // start
    for(unsigned i=0;i<8u;++i)v42_emit_bit(s,(c>>i)&1u);
    for(unsigned i=0;i<stop_bits;++i)v42_emit_bit(s,1);
}
void v42_flush_ones_to_octet(NetState& s) { while (s.v42_tx_bits) v42_emit_bit(s,1); }

inline std::uint8_t v42_seq_distance(std::uint8_t from,std::uint8_t to) noexcept {
    return static_cast<std::uint8_t>((static_cast<unsigned>(to)+128u-static_cast<unsigned>(from))&0x7Fu);
}

void v42_negotiate_xid(NetState& s,std::vector<std::uint8_t>& f) {
    // XID: format byte at [2], then groups {id,len_hi,len_lo,payload...}.
    // Keep V.42bis disabled, but honor LAPM frame/window limits negotiated by
    // the guest so the link layer does not outrun its receive window.
    if (f.size()<6 || f[2]!=0x82u) return;
    std::size_t g=3;
    while (g+3u<=f.size()) {
        const std::uint8_t gid=f[g];
        const std::size_t glen=(static_cast<std::size_t>(f[g+1])<<8u)|f[g+2];
        const std::size_t end=g+3u+glen;
        if (end>f.size()) break;
        std::size_t i=g+3u;
        while (i+2u<=end) {
            const auto pid=f[i], plen=f[i+1]; i+=2u;
            if (i+plen>end) break;
            std::uint32_t value=0;
            for (unsigned j=0;j<plen && j<4u;++j) value=(value<<8u)|f[i+j];
            if (gid==0xF0u && pid==1u && plen>=1u) {
                f[i]=0u; // reject V.42bis; LAPM remains active
            } else if (gid==0x80u && pid==5u && plen>0u && value>=8u) {
                const auto bytes=static_cast<unsigned>(value/8u);
                s.v42_tx_max_size=static_cast<std::uint16_t>(std::clamp(bytes,16u,512u));
            } else if (gid==0x80u && pid==7u && plen>0u && value>0u) {
                s.v42_tx_window=static_cast<std::uint8_t>(std::clamp(value,1u,31u));
            }
            i+=plen;
        }
        g=end;
    }
}

void v42_send_rr(NetState& s,std::uint8_t address,std::uint8_t pf=0) {
    v42_send_hdlc(s,{address,0x01u,static_cast<std::uint8_t>((s.v42_rx_seq<<1u)|(pf&1u))});
    ++s.v42_rr_frames;
}

void v42_reset_link_state(NetState& s,std::uint64_t now) {
    s.v42_tx_seq=s.v42_rx_seq=s.v42_tx_ack=0;
    s.v42_receiver_ready=true;
    s.v42_sent_valid.fill(false);
    for(auto& f:s.v42_sent_frames) f.clear();
    s.v42_last_activity_cycle=now;
    s.v42_last_keepalive_cycle=now;
}

bool v42_ack_to(NetState& s,std::uint8_t nr) {
    nr&=0x7Fu;
    const auto outstanding=v42_seq_distance(s.v42_tx_ack,s.v42_tx_seq);
    const auto advance=v42_seq_distance(s.v42_tx_ack,nr);
    if (advance>outstanding) return false; // impossible/future acknowledgement
    while (s.v42_tx_ack!=nr) {
        const auto seq=s.v42_tx_ack;
        if (s.v42_sent_valid[seq]) {
            s.v42_sent_valid[seq]=false;
            s.v42_sent_frames[seq].clear();
            ++s.v42_acked_frames;
        }
        s.v42_tx_ack=static_cast<std::uint8_t>((s.v42_tx_ack+1u)&0x7Fu);
    }
    return true;
}

void v42_retransmit_from(NetState& s,std::uint8_t nr) {
    nr&=0x7Fu;
    std::uint8_t seq=nr;
    unsigned guard=0;
    while(seq!=s.v42_tx_seq && guard++<128u) {
        if (s.v42_sent_valid[seq] && !s.v42_sent_frames[seq].empty()) {
            auto f=s.v42_sent_frames[seq];
            if (f.size()>=3u) f[2]=static_cast<std::uint8_t>(s.v42_rx_seq<<1u);
            v42_send_hdlc(s,f);
            ++s.v42_retransmits;
        }
        seq=static_cast<std::uint8_t>((seq+1u)&0x7Fu);
    }
}

void v42_handle_frame(NetState& s,DCRuntime& r,std::vector<std::uint8_t> frame) {
    if (frame.size()<2u) return;
    ++s.v42_frames_in;
    s.v42_last_activity_cycle=r.pvr_spg_sh4_cycles;
    const auto addr=frame[0], ctrl=frame[1];
    const auto uctrl=static_cast<std::uint8_t>(ctrl&0xEFu);
    if (uctrl==0x6Fu) { // SABME
        v42_send_hdlc(s,{addr,static_cast<std::uint8_t>(0x63u|(ctrl&0x10u))});
        s.v42_phase=V42Phase::Connected;
        v42_reset_link_state(s,r.pvr_spg_sh4_cycles);
        std::cout << "[V42] SABME -> UA | LAPM connected\n";
        return;
    }
    if (uctrl==0xAFu) { // XID
        v42_negotiate_xid(s,frame);
        v42_send_hdlc(s,frame);
        std::cout << "[V42] XID negotiated | V.42bis=off | tx-max=" << s.v42_tx_max_size
                  << " window=" << static_cast<unsigned>(s.v42_tx_window) << "\n";
        return;
    }
    if (uctrl==0x43u) { // DISC
        v42_send_hdlc(s,{addr,0x73u}); s.v42_phase=V42Phase::Release;
        std::cout << "[V42] DISC -> UA\n"; return;
    }
    if ((ctrl&0x03u)==0x01u && frame.size()>=3u) { // RR/RNR/REJ/SREJ
        const auto nr=static_cast<std::uint8_t>(frame[2]>>1u);
        const bool ack_ok=v42_ack_to(s,nr);
        const auto stype=static_cast<std::uint8_t>(ctrl&0x0Fu);
        if (stype==0x01u) { // RR
            s.v42_receiver_ready=true; ++s.v42_rr_frames;
        } else if (stype==0x05u) { // RNR
            s.v42_receiver_ready=false; ++s.v42_rnr_frames;
        } else if (stype==0x09u) { // REJ: go-back-N
            s.v42_receiver_ready=true; ++s.v42_rej_frames;
            v42_retransmit_from(s,nr);
        } else if (stype==0x0Du) { // SREJ - conservative single-frame retry
            ++s.v42_rej_frames;
            if (s.v42_sent_valid[nr] && !s.v42_sent_frames[nr].empty()) {
                auto f=s.v42_sent_frames[nr];
                if(f.size()>=3u)f[2]=static_cast<std::uint8_t>(s.v42_rx_seq<<1u);
                v42_send_hdlc(s,f); ++s.v42_retransmits;
            }
        }
        if (s.submit_trace_active || s.v42_frames_in<=16u) {
            std::cout << "[V42] S-frame ctrl=0x" << std::hex << static_cast<unsigned>(ctrl) << std::dec
                      << " nr=" << static_cast<unsigned>(nr)
                      << " ack=" << static_cast<unsigned>(s.v42_tx_ack)
                      << " tx=" << static_cast<unsigned>(s.v42_tx_seq)
                      << " ready=" << (s.v42_receiver_ready?1:0)
                      << (ack_ok?"":" invalid-ack") << "\n";
        }
        return;
    }
    if ((ctrl&1u)==0u && frame.size()>=3u) { // I-frame, modulo-128 extended control
        const auto ns=static_cast<std::uint8_t>(ctrl>>1u);
        const auto nr=static_cast<std::uint8_t>(frame[2]>>1u);
        v42_ack_to(s,nr); // I-frames piggyback acknowledgements too.
        if (s.v42_phase!=V42Phase::Connected) {
            s.v42_phase=V42Phase::Connected;
            std::cout << "[V42] accepting first I-frame as LAPM connected\n";
        }
        if (ns!=s.v42_rx_seq) {
            // REJ with the next sequence number we expect.
            v42_send_hdlc(s,{addr,0x09u,static_cast<std::uint8_t>(s.v42_rx_seq<<1u)});
            ++s.v42_rej_frames;
            if (s.submit_trace_active || s.v42_frames_in<=16u) std::cout << "[V42] out-of-order I-frame ns="<<(unsigned)ns<<" expected="<<(unsigned)s.v42_rx_seq<<"\n";
            return;
        }
        s.v42_rx_seq=static_cast<std::uint8_t>((s.v42_rx_seq+1u)&0x7Fu);
        for (std::size_t i=3u;i<frame.size();++i) ppp_serial_write(s,r,frame[i]);
        v42_send_rr(s,addr,frame[2]&1u);
        if (s.submit_trace_active || s.v42_frames_in<=16u) std::cout << "[V42] I-frame ns=" << static_cast<unsigned>(ns)
                      << " nr=" << static_cast<unsigned>(nr) << " payload=" << (frame.size()-3u) << " bytes\n";
        return;
    }
    if (s.submit_trace_active || s.v42_frames_in<=16u) std::cout << "[V42] control frame ctrl=0x" << std::hex << (unsigned)ctrl << std::dec << "\n";
}

void v42_decode_hdlc_bits(NetState& s,DCRuntime& r,const std::vector<std::uint8_t>& bits) {
    std::vector<std::uint8_t> bytes; bytes.reserve(bits.size()/8u);
    std::uint8_t cur=0; unsigned pos=0,ones=0; bool bad=false;
    for (auto bit8:bits) {
        const unsigned bit=bit8&1u;
        if (bit) {
            ++ones;
            if (ones>6u) { bad=true; break; }
        } else {
            if (ones==5u) { ones=0; continue; } // stuffed zero
            if (ones>=6u) { bad=true; break; }
            ones=0;
        }
        if (bit) cur|=static_cast<std::uint8_t>(1u<<pos);
        if (++pos==8u) { bytes.push_back(cur); cur=0; pos=0; }
    }
    if (bad || pos!=0u || bytes.size()<4u) { ++s.v42_abort_frames; return; }
    const auto got=static_cast<std::uint16_t>(bytes[bytes.size()-2u] | (static_cast<std::uint16_t>(bytes.back())<<8u));
    bytes.resize(bytes.size()-2u);
    if (got!=ppp_fcs16(bytes.data(),bytes.size())) {
        ++s.v42_crc_errors;
        if (s.submit_trace_active || s.v42_crc_errors<=8u) std::cout << "[V42] HDLC frame CRC mismatch | count=" << s.v42_crc_errors << "\n";
        return;
    }
    v42_handle_frame(s,r,std::move(bytes));
}

void v42_hdlc_wire_bit(NetState& s,DCRuntime& r,std::uint8_t bit) {
    bit&=1u;
    s.v42_raw_bits.push_back(bit);
    s.v42_flag_shift=static_cast<std::uint8_t>((s.v42_flag_shift>>1u)|(bit<<7u));
    if (s.v42_flag_bits<8u) ++s.v42_flag_bits;
    if (s.v42_flag_bits>=8u && s.v42_flag_shift==0x7Eu) {
        if (s.v42_raw_bits.size()>=8u) s.v42_raw_bits.resize(s.v42_raw_bits.size()-8u);
        if (s.v42_in_frame && !s.v42_raw_bits.empty()) v42_decode_hdlc_bits(s,r,s.v42_raw_bits);
        s.v42_raw_bits.clear(); s.v42_in_frame=true;
    }
    if (s.v42_raw_bits.size()>65536u) { s.v42_raw_bits.clear(); s.v42_in_frame=false; }
}

void v42_hdlc_wire_byte(NetState& s,DCRuntime& r,std::uint8_t v) {
    for (unsigned i=0;i<8u;++i) v42_hdlc_wire_bit(s,r,static_cast<std::uint8_t>((v>>i)&1u));
}

// During V.42 detection the first HDLC flag is a raw bit pattern, not a V.14
// character.  The real ChuChu stream can therefore switch to LAPM without ever
// producing a V.14-decoded 0x7e.  Scan the physical bitstream in parallel once
// an ADP response has been sent and hand the remainder of the byte directly to
// the HDLC decoder when the first flag is observed.
bool v42_detection_scan_hdlc_flag(NetState& s,DCRuntime& r,std::uint8_t v) {
    if (s.v42_phase!=V42Phase::Detection || !s.v42_adp_sent) return false;
    for (unsigned i=0;i<8u;++i) {
        const auto bit=static_cast<std::uint8_t>((v>>i)&1u);
        s.v42_flag_shift=static_cast<std::uint8_t>((s.v42_flag_shift>>1u)|(bit<<7u));
        if (s.v42_flag_bits<8u) ++s.v42_flag_bits;
        if (s.v42_flag_bits>=8u && s.v42_flag_shift==0x7Eu) {
            s.v42_phase=V42Phase::Establish;
            s.v42_raw_bits.clear();
            s.v42_in_frame=true;
            s.v42_flag_shift=0x7Eu;
            s.v42_flag_bits=8u;
            std::cout << "[V42] raw HDLC flag detected -> establish\n";
            for (unsigned j=i+1u;j<8u;++j)
                v42_hdlc_wire_bit(s,r,static_cast<std::uint8_t>((v>>j)&1u));
            return true;
        }
    }
    return false;
}

void v42_process_v14_char(NetState& s,std::uint8_t c) {
    if (s.v42_phase==V42Phase::None || s.v42_phase==V42Phase::Detection) {
        if ((c==0x11u || c==0x91u) && (s.v42_odp_count==0u || c!=s.v42_last_odp)) {
            s.v42_last_odp=c; ++s.v42_odp_count; s.v42_phase=V42Phase::Detection;
            if (s.v42_odp_count==4u) {
                // ADP response: EC with long mark intervals.  Keep the response
                // queue bounded while the originator switches from ODP to HDLC:
                // send immediately, then only one clean retry every 8 ODP groups.
                ++s.v42_adp_groups;
                const bool emit=!s.v42_adp_sent || ((s.v42_adp_groups-1u)%8u)==0u;
                if (emit) {
                    v42_emit_v14_char(s,'E',9u); v42_emit_v14_char(s,'C',9u); v42_flush_ones_to_octet(s);
                    if (!s.v42_adp_sent) std::cout << "[V42] ODP detected -> ADP EC\n";
                    else std::cout << "[V42] ODP retry -> ADP EC\n";
                    s.v42_adp_sent=true;
                } else if (s.v42_adp_groups==2u) {
                    std::cout << "[V42] suppressing duplicate ADP flood while waiting for HDLC flag\n";
                }
                s.v42_odp_count=0;
            }
            return;
        }
        if (s.v42_phase==V42Phase::Detection && c==0x7Eu) {
            s.v42_phase=V42Phase::Establish; s.v42_raw_bits.clear(); s.v42_in_frame=false; s.v42_flag_shift=0; s.v42_flag_bits=0;
            std::cout << "[V42] detection complete -> HDLC establish\n";
            return;
        }
        if (c!=0x11u && c!=0x91u) s.v42_odp_count=0;
    }
}

void v42_v14_wire_byte(NetState& s,DCRuntime& r,std::uint8_t v) {
    (void)r;
    for (unsigned i=0;i<8u;++i) {
        const unsigned bit=(v>>i)&1u;
        if (s.v14_rx_state==0) { // idle/mark
            if (!bit) { s.v14_rx_state=1; s.v14_rx_byte=0; s.v14_rx_bits=0; }
        } else if (s.v14_rx_state==1) {
            if (bit) s.v14_rx_byte|=static_cast<std::uint8_t>(1u<<s.v14_rx_bits);
            if (++s.v14_rx_bits==8u) s.v14_rx_state=2;
        } else { // first stop bit
            if (bit) { v42_process_v14_char(s,s.v14_rx_byte); s.v14_rx_state=0; }
            else { s.v14_rx_state=1; s.v14_rx_byte=0; s.v14_rx_bits=0; }
        }
    }
}

void v42_wire_write(NetState& s,DCRuntime& r,std::uint8_t v) {
    ++r.modem_tx_bytes;
    if (s.v42_probe.size()<64u) {
        s.v42_probe.push_back(v);
        if (s.v42_probe.size()==32u || s.v42_probe.size()==64u) {
            std::cout << "[V42] guest-wire[" << (s.v42_probe.size()-32u) << ".." << (s.v42_probe.size()-1u) << "]=";
            for (std::size_t i=s.v42_probe.size()-32u;i<s.v42_probe.size();++i) std::cout << std::hex << (unsigned)s.v42_probe[i] << (i+1u==s.v42_probe.size()?"":" ");
            std::cout << std::dec << " | ASYN=0\n";
        }
    }
    if (s.v42_phase==V42Phase::Establish || s.v42_phase==V42Phase::Connected || s.v42_phase==V42Phase::Release) {
        v42_hdlc_wire_byte(s,r,v);
    } else {
        // The real ChuChu stream exits ODP with a raw HDLC flag at arbitrary bit
        // alignment.  Detect that before asking the V.14 character decoder to
        // interpret the same byte.
        if (v42_detection_scan_hdlc_flag(s,r,v)) return;
        v42_v14_wire_byte(s,r,v);
        // Fallback for software that starts LAPM byte-aligned without ODP/ADP.
        if (s.v42_phase==V42Phase::None && v==0x7Eu) {
            s.v42_phase=V42Phase::Establish; s.v42_raw_bits.clear(); s.v42_in_frame=true; s.v42_flag_shift=0x7E; s.v42_flag_bits=8;
            std::cout << "[V42] direct HDLC flag fallback -> establish\n";
        }
    }
}

void v42_pump_to_guest(NetState& s,std::uint64_t now) {
    // Before V.42 detection starts, the remote modem keeps the synchronous
    // carrier in mark/idle state so the guest detector has a live line.
    if ((s.v42_phase==V42Phase::None || s.v42_phase==V42Phase::Detection) && s.backend_serial.empty()) {
        s.backend_serial.push_back(0xFFu);
        ++s.v42_idle_mark_bytes;
        if (!s.v42_idle_mark_announced) {
            s.v42_idle_mark_announced=true;
            std::cout << "[V42] carrier idle/mark stream -> guest (0xFF)\n";
        }
        return;
    }
    if (s.v42_phase==V42Phase::Connected && !s.v42_ppp_to_guest.empty() && s.backend_serial.size()<256u) {
        const auto outstanding=v42_seq_distance(s.v42_tx_ack,s.v42_tx_seq);
        if (s.v42_receiver_ready && outstanding<s.v42_tx_window) {
            const auto seq=s.v42_tx_seq;
            std::vector<std::uint8_t> f{1u,static_cast<std::uint8_t>(seq<<1u),static_cast<std::uint8_t>(s.v42_rx_seq<<1u)};
            const std::size_t n=std::min<std::size_t>(s.v42_tx_max_size,s.v42_ppp_to_guest.size());
            for (std::size_t i=0;i<n;++i) { f.push_back(s.v42_ppp_to_guest.front()); s.v42_ppp_to_guest.pop_front(); }
            s.v42_sent_frames[seq]=f;
            s.v42_sent_valid[seq]=true;
            s.v42_tx_seq=static_cast<std::uint8_t>((s.v42_tx_seq+1u)&0x7Fu);
            v42_send_hdlc(s,f);
            s.v42_last_activity_cycle=now;
            if (s.submit_trace_active) std::cout << "[V42] TX I-frame ns=" << static_cast<unsigned>(seq)
                      << " nr=" << static_cast<unsigned>(s.v42_rx_seq)
                      << " bytes=" << n << " out=" << static_cast<unsigned>(v42_seq_distance(s.v42_tx_ack,s.v42_tx_seq)) << "\n";
        }
    } else if (s.v42_phase==V42Phase::Connected && s.v42_ppp_to_guest.empty() && s.backend_serial.empty()
               && s.v42_last_activity_cycle!=0 && now-s.v42_last_activity_cycle>=kV42InactivityCycles
               && (s.v42_last_keepalive_cycle==0 || now-s.v42_last_keepalive_cycle>=kV42InactivityCycles)) {
        // LAPM T403-style poll keeps acknowledgement state moving on a quiet link.
        v42_send_rr(s,1u,1u);
        s.v42_last_keepalive_cycle=now;
        s.v42_last_activity_cycle=now;
        if (s.submit_trace_active) std::cout << "[V42] inactivity RR(P=1) nr=" << static_cast<unsigned>(s.v42_rx_seq) << "\n";
    } else if ((s.v42_phase==V42Phase::Establish || s.v42_phase==V42Phase::Connected) && s.backend_serial.empty()) {
        // Continuous HDLC flags are the idle state once LAPM establishment begins.
        v42_emit_flag(s); v42_emit_flag(s);
    }
}



void submit_trace_arm(DCRuntime& r,NetState& s,std::uint64_t now) {
    s.submit_trace_active=true;
    s.submit_trace_until=now+kSubmitTraceWindowCycles;
    s.submit_trace_next_sample=now;
    s.submit_trace_samples=0;
    s.submit_trace_tx_logs=0;
    s.submit_trace_last_pc=r.current_pc;
    s.trace_start_modem_tx=s.trace_last_modem_tx=r.modem_tx_bytes;
    s.trace_start_ppp_tx=s.trace_last_ppp_tx=r.modem_ppp_tx_frames;
    s.trace_start_ipv4=s.trace_last_ipv4=r.modem_ipv4_frames;
    s.trace_start_tcp=s.trace_last_tcp=r.modem_tcp_frames;
    s.trace_start_flash_program=s.trace_last_flash_program=r.bios_flash_program_calls;
    s.trace_start_vmu_writes=s.trace_last_vmu_writes=r.maple_vmu_writes;
    std::cout << "[SUBMIT TRACE] START down | pc=0x" << std::hex << r.current_pc << std::dec
              << " | maple-changes=" << r.maple_input_changes
              << " | tx=" << r.modem_tx_bytes << " ppp=" << r.modem_ppp_tx_frames
              << " ip=" << r.modem_ipv4_frames << " tcp=" << r.modem_tcp_frames << "\n";
}

void submit_trace_tick(DCRuntime& r,NetState& s,std::uint64_t now) {
    const auto buttons=static_cast<std::uint16_t>(r.maple_controller.buttons&0xFFFFu);
    const bool start_now=(buttons&(1u<<3u))!=0;
    const bool start_before=(s.last_maple_buttons&(1u<<3u))!=0;
    if (s.connect==ConnectState::Connected && start_now && !start_before) submit_trace_arm(r,s,now);
    s.last_maple_buttons=buttons;
    s.last_maple_changes=r.maple_input_changes;
    if (!s.submit_trace_active) return;
    if (now>=s.submit_trace_until) {
        std::cout << "[SUBMIT TRACE] window end | pc=0x" << std::hex << r.current_pc << std::dec
                  << " | dtx=" << (r.modem_tx_bytes-s.trace_start_modem_tx)
                  << " dppp=" << (r.modem_ppp_tx_frames-s.trace_start_ppp_tx)
                  << " dip=" << (r.modem_ipv4_frames-s.trace_start_ipv4)
                  << " dtcp=" << (r.modem_tcp_frames-s.trace_start_tcp)
                  << " dflash=" << (r.bios_flash_program_calls-s.trace_start_flash_program)
                  << " dvmu=" << (r.maple_vmu_writes-s.trace_start_vmu_writes)
                  << " | v42=" << static_cast<unsigned>(s.v42_tx_seq) << "/" << static_cast<unsigned>(s.v42_tx_ack)
                  << "/" << static_cast<unsigned>(s.v42_rx_seq)
                  << " out=" << static_cast<unsigned>(v42_seq_distance(s.v42_tx_ack,s.v42_tx_seq))
                  << " crc=" << s.v42_crc_errors << " abort=" << s.v42_abort_frames
                  << " rr=" << s.v42_rr_frames << " rnr=" << s.v42_rnr_frames << " rej=" << s.v42_rej_frames
                  << " retx=" << s.v42_retransmits << " acked=" << s.v42_acked_frames << "\n";
        s.submit_trace_active=false;
        return;
    }
    if (now<s.submit_trace_next_sample) return;
    s.submit_trace_next_sample=now+kSubmitTraceSampleCycles;
    const bool pc_region_changed=((r.current_pc^s.submit_trace_last_pc)&0xFFFFFF00u)!=0;
    const bool counters_changed=r.modem_tx_bytes!=s.trace_last_modem_tx || r.modem_ppp_tx_frames!=s.trace_last_ppp_tx ||
        r.modem_ipv4_frames!=s.trace_last_ipv4 || r.modem_tcp_frames!=s.trace_last_tcp ||
        r.bios_flash_program_calls!=s.trace_last_flash_program || r.maple_vmu_writes!=s.trace_last_vmu_writes;
    if ((pc_region_changed||counters_changed) && s.submit_trace_samples<96u) {
        ++s.submit_trace_samples;
        std::cout << "[SUBMIT TRACE] sample#" << s.submit_trace_samples
                  << " pc=0x" << std::hex << r.current_pc << std::dec
                  << " buttons=0x" << std::hex << buttons << std::dec
                  << " tx=" << r.modem_tx_bytes << " ppp=" << r.modem_ppp_tx_frames
                  << " ip=" << r.modem_ipv4_frames << " tcp=" << r.modem_tcp_frames
                  << " flash=" << r.bios_flash_program_calls << " vmu=" << r.maple_vmu_writes
                  << " | v42=" << static_cast<unsigned>(s.v42_tx_seq) << "/" << static_cast<unsigned>(s.v42_tx_ack)
                  << "/" << static_cast<unsigned>(s.v42_rx_seq)
                  << " out=" << static_cast<unsigned>(v42_seq_distance(s.v42_tx_ack,s.v42_tx_seq))
                  << " ready=" << (s.v42_receiver_ready?1:0)
                  << " crc=" << s.v42_crc_errors << " rr=" << s.v42_rr_frames << " rnr=" << s.v42_rnr_frames
                  << " rej=" << s.v42_rej_frames << " retx=" << s.v42_retransmits << "\n";
    }
    s.submit_trace_last_pc=r.current_pc;
    s.trace_last_modem_tx=r.modem_tx_bytes;
    s.trace_last_ppp_tx=r.modem_ppp_tx_frames;
    s.trace_last_ipv4=r.modem_ipv4_frames;
    s.trace_last_tcp=r.modem_tcp_frames;
    s.trace_last_flash_program=r.bios_flash_program_calls;
    s.trace_last_vmu_writes=r.maple_vmu_writes;
}

void begin_ppp(NetState& s,DCRuntime& r){if(s.ppp_started)return;s.ppp_started=true;udp_endpoint(s,kChuChuUdpPort);s.v42_phase=V42Phase::None;s.v14_rx_state=0;s.v14_rx_bits=0;s.v42_raw_bits.clear();s.v42_ppp_to_guest.clear();s.v42_tx_byte=0;s.v42_tx_bits=0;s.v42_tx_seq=s.v42_rx_seq=s.v42_tx_ack=0;s.v42_receiver_ready=true;s.v42_tx_max_size=128u;s.v42_tx_window=15u;s.v42_sent_valid.fill(false);for(auto& f:s.v42_sent_frames)f.clear();s.v42_crc_errors=s.v42_abort_frames=s.v42_rr_frames=s.v42_rnr_frames=s.v42_rej_frames=s.v42_retransmits=s.v42_acked_frames=0;s.v42_last_activity_cycle=s.v42_last_keepalive_cycle=0;s.v42_probe.clear();s.v42_idle_mark_bytes=0;s.v42_idle_mark_announced=false;s.v42_adp_sent=false;s.v42_adp_groups=0;std::cout<<"[NET] modem carrier online | transport="<<(((s.regs[8]&0x80u)==0)?"V42/LAPM":"ASYNC")<<" | DC 192.168.167.2 <-> host 192.168.167.1 | DNS 46.101.91.123 | UDP 9789 bound\n";mirror_diag_full(r,s);}
void check_handshake(DCRuntime& r,NetState& s){const bool dtr=(s.regs[9]&1u)!=0;const bool data=(s.regs[9]&4u)!=0;const bool aut=(s.regs[0x15]&8u)!=0;if(dtr&&(data||aut)&&(s.connect==ConnectState::Ringing||s.connect==ConnectState::NegoComplete)){s.connect=ConnectState::Handshaking;s.deadline=r.pvr_spg_sh4_cycles+kSh4Clock/1000u;std::cout<<"[MODEM] DTR -> HANDSHAKING\n";mirror_diag_full(r,s);}}

void normal_write(DCRuntime& r,NetState& s,std::uint32_t reg,std::uint8_t data){auto old=s.regs[reg];s.regs[reg]=static_cast<std::uint8_t>((old&~kWriteMask[reg])|(data&kWriteMask[reg]));switch(reg){case 0x02:set_status(s,0x0F,0x02,(s.regs[2]&8u)&&s.connect==ConnectState::Connected);break;case 0x08:if((old^s.regs[8])&0x80u)std::cout<<"[MODEM] transport mode -> "<<((s.regs[8]&0x80u)?"ASYNC":"V42/LAPM")<<"\n";break;case 0x09:check_handshake(r,s);if(s.connect==ConnectState::Connected){set_status(s,0x0F,0x10,(s.regs[9]&1u)!=0);if(!(s.regs[9]&1u))set_status(s,0x0F,0x80,false);}break;case 0x10:if(s.module_download){s.download_crc=static_cast<std::uint8_t>((s.download_crc<<1u)+((s.download_crc&0x80u)>>7u)+data);}else if(s.connect==ConnectState::Disconnected||s.connect==ConnectState::Dialing){if(s.connect==ConnectState::Disconnected){s.connect=ConnectState::Dialing;s.regs[0x16]=0;s.last_dial_cycle=r.pvr_spg_sh4_cycles;s.deadline=r.pvr_spg_sh4_cycles+kSh4Clock;std::cout<<"[MODEM] DISCONNECTED -> DIALING\n";}++r.modem_dial_bytes;s.regs[0x1E]|=8u;}else if(s.connect==ConnectState::Connected&&(s.regs[8]&1u)){if(s.regs[8]&0x80u){++r.modem_tx_bytes;ppp_serial_write(s,r,data);}else v42_wire_write(s,r,data);s.regs[0x1E]&=static_cast<std::uint8_t>(~8u);s.tx_fifo_size+=1.f;if(s.submit_trace_active&&s.submit_trace_tx_logs<16u){++s.submit_trace_tx_logs;std::cout<<"[SUBMIT TRACE] TBUFFER byte#"<<s.submit_trace_tx_logs<<" pc=0x"<<std::hex<<r.current_pc<<" value=0x"<<(unsigned)data<<std::dec<<"\n";}}break;case 0x14:if(data==0x4F||data==0x5F){s.reg1b_saved=s.regs[0x1B];s.regs[0x1B]=data;s.module_download=true;s.download_crc=0;}else if(data==0&&s.module_download){s.regs[0x17]=0xFF;s.regs[0x16]=s.download_crc;s.regs[0x1B]=s.reg1b_saved;s.module_download=false;}break;case 0x15:check_handshake(r,s);break;case 0x19:s.word_dsp_write=true;break;case 0x1D:if((s.regs[0x1D]&0x20u)&&!(old&0x20u))s.word_dsp_write=false;if(s.regs[0x1D]&0x80u){s.regs[0x1D]&=0x7Fu;s.regs[0x1F]|=0x08u;std::uint32_t a=s.regs[0x1C]|((s.regs[0x1D]&0x0Fu)<<8u);if(a<s.dsp.size()){if(s.regs[0x1D]&0x20u){s.dsp[a]=s.regs[0x18];if(s.word_dsp_write&&a+1<s.dsp.size())s.dsp[a+1]=s.regs[0x19];if((a==0x26B||a==0x26C)&&s.regs[0x18])s.dsp[0x26F]=1;}else{s.regs[0x18]=s.dsp[a];s.regs[0x19]=(a+1<s.dsp.size())?s.dsp[a+1]:0;}}}break;case 0x1F:if(!(s.regs[0x1F]&0x04u))s.regs[0x1F]&=static_cast<std::uint8_t>(~0x40u);if(s.regs[0x1F]&1u){if(s.regs[0x1A]&0x80u){s.regs[0x1A]&=0x7Fu;hard_reset(r,s,0);hard_reset(r,s,1);s.regs[0x1F]|=1u;s.regs[0x20]=0;}else{s.regs[0x1F]&=static_cast<std::uint8_t>(~1u);if(s.regs[0x1F]&4u)s.regs[0x1F]|=0x40u;}if(s.regs[0x12]==0x81)s.v8bis=(s.dsp[0x439]&1u)!=0;}if(!(old&0x08u))s.regs[0x1F]&=static_cast<std::uint8_t>(~0x08u);if(!(s.regs[0x1F]&0x08u))s.regs[0x1F]&=static_cast<std::uint8_t>(~0x80u);break;default:break;}refresh_fifo(r,s);mirror_diag_fast(r,s);}

void connected_step(DCRuntime& r,NetState& s){
    const auto now=r.pvr_spg_sh4_cycles;
    if (s.next_host_poll_cycle==0 || now>=s.next_host_poll_cycle) {
        poll_udp(s,r); poll_tcp(s,r);
        s.next_host_poll_cycle=now+kHostSocketPollCycles;
        ++s.socket_poll_batches;
    }
    if (now<s.next_serial_cycle) return;
    const auto step=std::max<std::uint32_t>(1u,s.cycles_per_byte);
    unsigned budget=16u;
    do {
        s.tx_fifo_size=(std::max)(0.f,s.tx_fifo_size-s.tx_rx_ratio);
        if(s.tx_fifo_size==0.f)s.regs[0x1E]|=8u;
        if((s.regs[8]&0x80u)==0)v42_pump_to_guest(s,now);
        if(s.rx_fifo.size()<16&&!s.backend_serial.empty()){s.rx_fifo.push_back(s.backend_serial.front());s.backend_serial.pop_front();}
        s.next_serial_cycle=(s.next_serial_cycle==0?now:s.next_serial_cycle)+step;
    } while(--budget && now>=s.next_serial_cycle);
    refresh_fifo(r,s);
}

void plugin_tick(DCRuntime& r){
    if(!r.network_enabled)return;
    auto* sp=find_state(r); if(!sp)return; auto& s=*sp;
    const auto now=r.pvr_spg_sh4_cycles;
    submit_trace_tick(r,s,now);
    if(s.state==ModemState::DspTest&&now>=s.deadline)dsp_test_start(r,s);
    else if(s.state==ModemState::DspEnd&&now>=s.deadline){s.state=ModemState::Normal;normal_defaults(r,s);}
    if(s.state!=ModemState::Normal){mirror_diag_fast(r,s);return;}
    s.regs[0x1F]&=static_cast<std::uint8_t>(~1u);
    if (s.connect==ConnectState::Connected) {
        const bool serial_due=(s.next_serial_cycle==0 || now>=s.next_serial_cycle);
        const bool host_due=(s.next_host_poll_cycle==0 || now>=s.next_host_poll_cycle);
        const bool diag_due=(s.next_full_diag_cycle==0 || now>=s.next_full_diag_cycle);
        const bool trace_due=s.submit_trace_active && now>=s.submit_trace_next_sample;
        if(!serial_due && !host_due && !diag_due && !trace_due){++s.tick_fast_returns;mirror_diag_fast(r,s);return;}
        connected_step(r,s);
        if(diag_due){s.next_full_diag_cycle=now+kFullDiagSyncCycles;mirror_diag_full(r,s);}else mirror_diag_fast(r,s);
        return;
    }
    switch(s.connect){
        case ConnectState::Dialing:
            if(now>=s.deadline){s.connect=ConnectState::Ringing;s.deadline=0;s.regs[0x1F]|=0x08u;if(!(s.regs[9]&4u)){set_status(s,0x0F,0x08,true);set_status(s,0x0B,0x10,true);}std::cout<<"[MODEM] DIALING -> RINGING\n";}break;
        case ConnectState::Handshaking:
            if(now>=s.deadline){if(s.regs[0x12]==0xAA)s.dsp[0x302]|=8u;s.regs[0x1F]|=8u;set_status(s,0x0F,0x08,false);set_status(s,0x0B,0x10,false);s.connect=ConnectState::PreConnected;s.deadline=now+kSh4Clock/2u;std::cout<<"[MODEM] HANDSHAKING -> PRE_CONNECTED\n";}break;
        case ConnectState::PreConnected:
            if(now>=s.deadline){if(s.regs[3]&0x10u)set_status(s,0x0F,0x80,true);if(s.regs[0x12]==0xAA){s.dsp[0x302]|=0x10u;s.dsp[0x302]=static_cast<std::uint8_t>((s.dsp[0x302]&0x1F)|(s.dsp[0x304]&0xE0));s.dsp[0x301]|=0x10;s.dsp[0x303]|=0xE1;s.dsp[0x2E3]=5;if((s.dsp[0x6A3]&1u)&&(s.dsp[0x6A2]&0x20u)){s.regs[0x12]=static_cast<std::uint8_t>(0xE0|(s.dsp[0x6A2]&0x1F));s.dsp[0x6C1]=0xFF;s.dsp[0x6C2]=0xFF;s.dsp[0x6C3]=0x3F;}else s.regs[0x12]=s.dsp[0x309];s.dsp[0x239]=12;}const bool valid=configure_speed(s);if(s.regs[0x15]&8u){s.regs[9]|=4u;s.regs[0x15]&=static_cast<std::uint8_t>(~8u);}if(valid){s.regs[0x14]=0;set_status(s,0x0F,0x10,true);if(s.regs[2]&8u)set_status(s,0x0F,0x02,true);set_status(s,0x0F,0x40,true);s.connect=ConnectState::Connected;s.next_serial_cycle=now;s.next_host_poll_cycle=now;s.next_full_diag_cycle=now;begin_ppp(s,r);std::cout<<"[MODEM] PRE_CONNECTED -> CONNECTED\n";}else{s.regs[0x14]=1;s.connect=ConnectState::Disconnected;}s.regs[0x1F]|=8u;s.dsp[0x208]=0xFF;s.dsp[0x209]=0xBF;}break;
        default:break;
    }
    refresh_fifo(r,s); mirror_diag_fast(r,s);
}


void ensure_online_flash(DCRuntime& r, NetState& s) {
    if (s.flash_repaired || !r.bios_flash_initialized) return;
    s.flash_repaired = true;
    constexpr std::size_t id = 0x1A058u;
    constexpr std::size_t mirror = id + 0xA0u;
    if (mirror + 8u >= r.bios_flash.size()) return;
    bool blank = true;
    for (std::size_t i=0;i<6u;++i) blank &= r.bios_flash[id+i] == 0xFFu;
    if (blank) {
        // Deterministic nonzero local console ID. It remains stable after persistence.
        const std::array<std::uint8_t,6> seed{0x44,0x43,0x52,0x31,0x32,0x01};
        for (std::size_t i=0;i<6u;++i) r.bios_flash[id+i]=seed[i];
    }
    std::uint8_t sum=0u;
    for (std::size_t i=0;i<6u;++i) {
        sum=static_cast<std::uint8_t>(sum+r.bios_flash[id+i]);
        r.bios_flash[mirror+i]=r.bios_flash[id+i];
    }
    r.bios_flash[id-1u]=r.bios_flash[mirror-1u]=sum;
    r.bios_flash[id-2u]=r.bios_flash[mirror-2u]=static_cast<std::uint8_t>(~sum);
    r.bios_flash[id+7u]=r.bios_flash[mirror+7u]=0xFEu;
    r.bios_flash_dirty=true;
    if (!r.bios_flash_path.empty()) {
        std::ofstream f(r.bios_flash_path, std::ios::binary|std::ios::trunc);
        if (f) {
            f.write(reinterpret_cast<const char*>(r.bios_flash.data()), static_cast<std::streamsize>(r.bios_flash.size()));
            if (f) { r.bios_flash_dirty=false; ++r.bios_flash_save_calls; }
        }
    }
    std::cout << "[NET FLASH] ChuChu console-id/checksum + mirror repaired lazily at first modem access\n";
}

NetState& activate_modem(DCRuntime& r) {
    NetState* existing=find_state(r);
    if (existing) { ensure_online_flash(r,*existing); return *existing; }
    if (!net_init()) std::cerr << "[NET] host socket initialization failed; modem remains visible\n";
    auto& s=state_for(r);
    s.last_maple_buttons=static_cast<std::uint16_t>(r.maple_controller.buttons&0xFFFFu);
    s.last_maple_changes=r.maple_input_changes;
    ensure_online_flash(r,s);
    std::cout << "[MODEM] lazy hardware activation on first MMIO access\n";
    mirror_diag_full(r,s);
    return s;
}

std::uint8_t read8(DCRuntime& r,std::uint32_t address){
    ++r.modem_reads; auto& s=activate_modem(r);
    const std::uint32_t raw=(physical29(address)-kModemPhysicalBase)>>2u;
    if(raw<0x100u)return(raw&1u)?0x10u:0x02u;
    const std::uint32_t reg=raw-0x100u; if(reg>=0x21u)return 0;
    if(s.state==ModemState::Normal){
        // Hot MMIO path: status/FIFO state is maintained when it changes, not on every poll.
        if(reg==0x0Fu)set_status(s,0x0F,0x20,(s.regs[8]&1u)&&s.connect==ConnectState::Connected);
        else if(reg==0x0Bu){const bool tone=s.connect==ConnectState::Disconnected||s.connect==ConnectState::NegoComplete;set_status(s,0x0B,0x80,tone);set_status(s,0x0B,0x40,tone);set_status(s,0x0B,0x20,tone);}
        auto v=s.regs[reg];
        if(reg==0&&!s.rx_fifo.empty()){s.rx_fifo.pop_front();refresh_fifo(r,s);mirror_diag_fast(r,s);}
        else ++s.mmio_fast_reads;
        return v;
    }
    if(s.state==ModemState::ControllerTest||s.state==ModemState::DspTest||s.state==ModemState::DspEnd)return reg==0x10?0:s.regs[reg];
    return 0;
}

void write8(DCRuntime& r,std::uint32_t address,std::uint8_t value){++r.modem_writes;auto& s=activate_modem(r);const std::uint32_t raw=(physical29(address)-kModemPhysicalBase)>>2u;if(raw<0x100u)return;const std::uint32_t reg=raw-0x100u;if(reg>0x20u)return;if(reg==0x20u){hard_reset(r,s,value);return;}if(s.state==ModemState::Normal)normal_write(r,s,reg,value);else if(s.trace_count++<32)std::cout<<"[MODEM] write reg 0x"<<std::hex<<reg<<"=0x"<<(unsigned)value<<std::dec<<" ignored until reset/self-test completes\n";}

bool plugin_startup(DCRuntime* r){if(!r)return false;std::cout<<"[PLUGIN network] cpp34s V42/LAPM RELIABILITY loaded | cpp34r fastpath + LAPM ARQ/window/RR-RNR-REJ | cpp34g guest ABI preserved\n";return true;}
void plugin_shutdown(DCRuntime* r){if(!r)return;std::lock_guard<std::mutex> l(g_states_mutex);auto it=g_states.find(r);if(it!=g_states.end()){close_network(*it->second);g_states.erase(it);}if(g_tls_runtime==r){g_tls_runtime=nullptr;g_tls_state=nullptr;}}
void plugin_tick_cb(DCRuntime* r){if(r)plugin_tick(*r);}bool plugin_handles(std::uint32_t a){auto p=physical29(a);return p>=0x00600000u&&p<0x00600800u;}bool plugin_read8_cb(DCRuntime* r,std::uint32_t a,std::uint8_t* v){if(!r||!v||!plugin_handles(a))return false;*v=read8(*r,a);return true;}bool plugin_write8_cb(DCRuntime* r,std::uint32_t a,std::uint8_t v){if(!r||!plugin_handles(a))return false;write8(*r,a,v);return true;}

const DCRPluginDescriptorV1 kPlugin{DCR_PLUGIN_ABI_V1,sizeof(DCRPluginDescriptorV1),DCR_PLUGIN_KIND_NETWORK,0u,"DreamcastRecompiled Network","cpp34s-v42-lapm-reliability-abi1",&plugin_startup,&plugin_shutdown,&plugin_tick_cb,&plugin_handles,&plugin_read8_cb,&plugin_write8_cb};

} // namespace
} // namespace dcrecomp_generated
DCR_PLUGIN_EXPORT const dcrecomp_generated::DCRPluginDescriptorV1* dcr_plugin_query_v1(){return &dcrecomp_generated::kPlugin;}
