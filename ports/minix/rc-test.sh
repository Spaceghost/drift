#!/bin/sh
# Drift test image only: replaced donor executables; licenses stay separate.
(
 echo DRIFT_MINIX_GUEST_START; uname -a; echo DRIFT_MINIX_NOBEEP=`sysenv nobeep`
 /usr/bin/calendar; echo DRIFT_MINIX_LINUX_MODEL_EXIT=$?
 /usr/bin/cal; echo DRIFT_MINIX_WINDOWS_MODEL_EXIT=$?
 /usr/bin/csplit; echo DRIFT_MINIX_SHARED_MODEL_EXIT=$?
 cat >/tmp/drift-pio.conf <<'CFG'
service fetch {
 system DEVIO;
 ipc SYSTEM pm rs tty ds vm vfs;
 io 42:2 61;
 uid 0;
};
CFG
 service up /usr/bin/fetch -label driftpio -config /tmp/drift-pio.conf
 echo DRIFT_MINIX_PIO_UP_EXIT=$?
 n=0
 while [ ! -f /tmp/drift-pio.status ] && [ $n -lt 30 ]; do sleep 1; n=`expr $n + 1`; done
 if test -f /tmp/drift-pio.status && grep -q PASS /tmp/drift-pio.status; then echo DRIFT_MINIX_PIO_STATUS=PASS; else echo DRIFT_MINIX_PIO_STATUS=FAIL; fi
 service down driftpio; echo DRIFT_MINIX_PIO_DOWN_EXIT=$?
 echo DRIFT_MINIX_ALL_DONE
) >/dev/tty00 2>&1
