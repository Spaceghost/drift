#!/bin/sh
# Test-image hook only. The base ISO and SDK remain SHA-pinned and unmodified.
(
 echo DRIFT_MINIX_RNG_GUEST_START; uname -a
 cat >/tmp/drift-rng.conf <<'CFG'
service fetch {
 system UMAP;
 ipc SYSTEM pm rs tty ds vm vfs pci;
 pci device 1af4:1044;
 uid 0;
};
CFG
 service up /usr/bin/fetch -label driftrng -config /tmp/drift-rng.conf
 echo DRIFT_MINIX_RNG_UP_EXIT=$?
 n=0
 while [ ! -f /tmp/drift-rng.status ] && [ $n -lt 30 ]; do sleep 1; n=`expr $n + 1`; done
 if test -f /tmp/drift-rng.status && grep -q PASS /tmp/drift-rng.status; then
   echo DRIFT_MINIX_RNG_STATUS=PASS
   service down driftrng; echo DRIFT_MINIX_RNG_DOWN_EXIT=$?
 else
   echo DRIFT_MINIX_RNG_STATUS=FAIL
   echo DRIFT_MINIX_RNG_SERVICE_RETAINED=1
 fi
 echo DRIFT_MINIX_RNG_ALL_DONE
) >/dev/tty00 2>&1
