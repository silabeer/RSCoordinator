#!/usr/bin/env python

import sys

num = int(sys.argv[1])
with open('Procfile.%d' % num, "w+") as f:
    for i in range(num):
        port = 7000 + i
        f.write("redis-%d: redis-server ./common.conf --port %d --cluster-config-file %d.conf \
                --dbfilename %d.rdb \
                --loadmodule ../src/module-oss.so PARTITIONS %d\n"
                % (port, port, port, port, num, port))

print("bootstrap command:")
print ("./bin/redis-trib.rb create --replicas 0 %s" %
       " ".join((("127.0.0.1:%d" % (7000 + i)) for i in range(num))))
