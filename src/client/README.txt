pebble client
----------------------------------
本目录为Pebble给game server提供的SDK参考实现，支持RPC、路由、名字功能，使用体验和PebbleServer一致。
pebble client只依赖pebble framework，可以pebble extension配合使用

                 user application
    --------------------------------------------
        pebble client     |  pebble extension
    --------------------------------------------
      pebble framework
    ----------------------|
        pebble common     |  zookeeper tbuspp


