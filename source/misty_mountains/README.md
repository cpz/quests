# The Misty Mountains

tcp echo client && server in C++
----

##### Misty Mountains/client
Arguments:
- -h [param] or =host [param] -- Hostname to connect. By default 127.0.0.1
- -p [param] or =port [param] -- Port to connect. By default 1337
  
##### Misty Mountains/server
Arguments:
- -x [param] or =xml [param] -- Path to XML Configuration file. By default 'config.xml' near executable
- -p [param] or =port [param] -- Port to connect. By default 1337
- -f [param] or =prefix [param] -- Prefix to add to echo. By default none.
- -s [param] or =suffix [param] -- Suffix to add to echo. By default none.

Notice: XML configuration is prefered and will be used over args. 

Libraries:
----

- [kissnet](https://github.com/Ybalrid/kissnet)
- [tinyxml2](https://github.com/leethomason/tinyxml2)
- [args](https://github.com/Taywee/args)


License
----

MIT