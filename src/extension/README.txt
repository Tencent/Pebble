pebble extension
----------------------------------
本目录为Pebble框架的扩展实现，如基于zookeeper的名字服务，基于tbuspp的消息队列等
pebble extension依赖pebble framework，可以独立使用扩展的功能部分，也可以配合pebble server一起使用。


                 user application
    --------------------------------------------  
        pebble server     |  pebble extension
    --------------------------------------------
      pebble framework    
    ----------------------|
        pebble common     |  zookeeper tbuspp
