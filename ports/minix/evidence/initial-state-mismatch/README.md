# Rejected comparison: unequal initial gate state

This run passed both guest models, the actual mode/gate checks, unrelated-port
denial and service cleanup. Its benchmark parity check correctly failed.

The first imported trial followed a suspend test, so its first port `0x61`
read saw gate bits 0. The later reference/imported trials started with gate
bits 3. `DRIFT_MINIX_TRACE_BAD index=7 kind=82 port=97 value=0` identified the
first read. The timing samples here are **not** a valid overhead comparison.

The verified harness suspends and checks gate bits 0 before **each** variant,
then resets the digest/timer. Its independent golden transcript includes the
first read as 0 and subsequent reads as 3. Raw failed results are retained.
