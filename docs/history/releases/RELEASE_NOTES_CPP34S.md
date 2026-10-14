# DreamcastRecomp v1.2 cpp34s - V42/LAPM RELIABILITY

Base: cpp34r NETWORK_FASTPATH_SUBMIT_TRACE.

## Motivo
El log real de ChuChu despues de START mostro `dtx=7562` pero `dppp=0 dip=0 dtcp=0`.
La entrada y el modem seguian vivos, pero el trafico no cruzaba de la linea V.42/LAPM a PPP.

## Cambios
- conserva todos los fast paths de cpp34r;
- ACK acumulativo LAPM N(R) modulo 128;
- tracking `tx_seq / tx_ack / rx_seq`;
- ventana de transmision negociada por XID;
- maximo de informacion TX negociado por XID;
- RR: confirma y habilita receptor;
- RNR: pausa nuevos I-frames;
- REJ: Go-Back-N y retransmision de I-frames pendientes;
- SREJ conservador para un frame;
- I-frames salientes almacenados hasta ACK;
- RR(P=1) de inactividad (~1 s) para mantener el enlace avanzando;
- SUBMIT TRACE ahora muestra estado LAPM, outstanding, CRC, RR/RNR/REJ y retransmisiones;
- contador de errores/abortos HDLC visible durante submit.

## ABI / guest
No cambia DCRuntime ABI1 ni el emitter. No regenera las 5185 funciones de ChuChu.
El overlay Late-AOT 0x8C036102 se conserva.
