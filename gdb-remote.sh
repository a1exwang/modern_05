#!/bin/bash
gdb -ex "file kernel" -ex "target remote :1234"
