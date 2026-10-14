# cpp34n — V42 RAW-FLAG ESTABLISH

Targeted follow-up to cpp34m based on the real ChuChu Rocket online trace.

The cpp34m trace proved the remote idle/mark bootstrap worked: the guest began
ODP and accepted enough of the response to stop repeating ODP. However PPP
remained at zero because the implementation only left V.42 Detection when
`0x7e` appeared as a *V.14 decoded character*.

In a real synchronous transition the first LAPM/HDLC flag is the raw bit
pattern `01111110` and it may be unaligned to the host byte boundary. cpp34n
therefore scans the physical bitstream during Detection after ADP has been sent,
then switches immediately to the HDLC decoder and preserves the remaining bits
of the transition byte.

Also limits duplicate ADP responses: one immediate `EC`, then a clean retry only
after 8 additional ODP groups. This prevents the response queue from being
flooded while the guest changes protocol.

No DCRuntime fields changed. Guest ABI remains 1 and existing ChuChu generated
code is reused.
