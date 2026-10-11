# Samsung SDM450 SUSFS compatibility patches

This directory contains kernel-tree-specific fixes used by `.github/workflows/BUILD-NOCTURNE.yml`.

`nocturne_susfs_49fix.patch` restores the SUSFS mount declarations that the upstream Linux 4.9 patch cannot place automatically because this Samsung tree has an extra `linux/fslog.h` include.

The workflow keeps SUSFS KSTAT, cmdline spoofing, open redirect, and SUS_MAP disabled for now because their upstream hunks target APIs/context not present in this kernel tree. They should be ported and validated independently before enabling them.
