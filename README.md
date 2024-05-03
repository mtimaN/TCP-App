### Mantu Matei-Cristian 322CA
## Tema 2 - Aplicatie client-server TCP si UDP pentru gestionarea mesajelor

### General Description

- The server acts as a middle man between the TCP and UDP clients.
- The TCP clients can subscribe and unsubscribe to any topic in order to receive notifications from the server.
- The UDP clients are the ones providing the content divided by topics. The server forwards the content to the subscribers of each topic.

### Technical aspects

- The server accepts any number of clients(not considering hardware limitations), using poll for I/O multiplexing.

- For message framing, I made the following protocol: an `int32_t` value (`buf_len`) is first sent, representing the length of the buffer. Then, exactly `buf_len` bytes are sent, thus achieving variable length buffer send and receive with a small overhead of 4 bytes(not considering the overhead of sending a TCP segment). I used the `send_all` and `recv_all` functions from Lab 7 (TCP) in order to be able to send data correctly regardless of the length of the TCP buffer and avoid segmentation problems.

- Information regarding each TCP client is stored by the server inside a `std::vector` of type `subscriber_t`. This allows the connection of any number of clients(again, not considering hardware limitations). Furthermore, information regarding subscribed topics is preserved upon disconnecting and reconnecting any number of times.

- For subscribing/unsubscribing, the client modifies the buffer where it receives the input from user by prepending either an `s` or an `u` before the topic name. This way, no allocations are made and the overhead is minimal.

- Regarding the parsing and formatting of the messages from UDP clients, I decided to only parse the topic inside server, while the subscriber is responsible with formatting the message to be printed to STDOUT for the user. The thought process behind this decision was that by doing so, less bytes of data are sent on the network. The content of the UDP message is prepended by the ip and port of the UDP client that sent it, in order to compactly send this information.

- For pattern matching, I used `strtok` to parse the topics by the delimiter `/`. I had to duplicate the strings each topic string as `strtok` "destroys" the original string. I used `strdup` as I consider it to be more elegant than `malloc`-ing. I `free`'d each buffer allocated using `strdup` in order to avoid memory leaks.

### Notes

- I used C++ because I wanted to make use of the STL library. Otherwise, I consider to have maintained a C-like implementation. I initially considered `unordered_map`(O(1) retrieval) for storing the topics each client is subscribed to but I believe it would not bring a consistent improvement because of the wildcard subscriptions.

- If there is one thing I would change regarding this homework for next year, I think it would be great if the checker could print STDERR strings from the app. I spent a long time debugging an error I couldn't understand because the checker did not forward the output of DIE.
