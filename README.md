# C++ Event-driven HTTP Server

A C++ single-threaded event-driven HTTP Server using an edge-triggered epoll loop with non-blocking IO.

## Table of contents

  - [Setup](#setup)
  - [Example usage](#example-usage)
  - [Implementation details](#implementation-details)
  - [Performance benchmarks](#performance-benchmarks)
  - [Comparisons to Node.js](#comparisons-to-nodejs)
  - [Design decisions](#design-decisions)
  - [Technology](#technology)
  - [Resources](#resources)
  - [Contact](#contact)

## Setup

Clone the [repository](https://github.com/tanjiaxian99/http-server) onto your local machine. Ensure that `cmake` is installed with version >= 3.7 together with `C++17`.

To build the project, run the following in the root directory:
```bash
mkdir build && cd build && cmake .. && make -j4
```

To run the server, run `./server`. The server should show the lines:
```
[INFO] Listening on ADDRESS: 0.0.0.0, PORT: 8080
[INFO] Waiting for new connection
```

Test the server by sending a cURL request to `http://localhost:8080/hello`, which should return `Hello World!`.
```bash
curl 'http://localhost:8080/hello'
```

The other available endpoints are `/echo` which echoes back the POST request body, and `/params` which takes the query parameters from the request and pass them back in the response body. Try them out with the following commands:
```bash
curl 'http://localhost:8080/echo' --data 'This should be echoed back'
```

```bash
curl 'http://localhost:8080/params?username=abc&password=1234'
```

## Example usage
Handlers can be found in `src/main.cpp`. New handlers can be added in the same format.

```cpp
void hello_handler(http::Request& request, http::Response& response) {
    response.set_body("Hello World!");
}

void echo_handler(http::Request& request, http::Response& response) {
    response.set_body(request.get_body());
}

void params_handler(http::Request& request, http::Response& response) {
    std::ostringstream oss;
    for (auto& [key, value] : request.get_url_params()) {
        oss << key << ": " << value << '\n';
    }
    response.set_body(oss.str());
}

int main() {
    HttpServer server = http::HttpServer();
    server.AddHandler("/hello", hello_handler);
    server.AddHandler("/echo", echo_handler);
    server.AddHandler("/params", params_handler);

    server.Start();
    return 0;
}
```

## Implementation details
The directory tree is shown below.
```
.
├── CMakeLists.txt
├── README.md
└── src
    ├── event_loop.cpp
    ├── event_loop.h
    ├── http_client.cpp
    ├── http_client.h
    ├── http_message.cpp
    ├── http_message.h
    ├── http_method.h
    ├── http_server.cpp
    ├── http_server.h
    ├── http_status.h
    ├── logger.cpp
    ├── logger.h
    └── main.cpp
```

Summary of files:
- `event_loop` contains a singleton class `EventLoop` that constructs an edge-triggered epoll loop with non-blocking IO.
- `http_client` stores the information pertaining to each individual HTTP client, such as the request and response.
- `http_message` contains the classes `Request` and `Response` which are the parsed versions of a HTTP request and response. `Request` has methods that breaks down the raw input string into the different sections of a request, while `Response` has methods that translate response fields into a response string.
- `http_method` stores the enum class `HTTPMethod` mapping to GET and POST methods.
- `http_server` contains all the relevant code to set up the server, from binding and listening to accepting and routing requests. 
- `http_status` has a single unordered map that maps status codes to their messages.
- `logger` contains a `Logger` class that simplifies logging.
- `main` is where the handlers and the server set up is done.

## Performance benchmarks
The open-source tool [wrk](https://github.com/wg/wrk) is used to stress-test the server. The server was capable of handling 10,000 concurrent HTTP connections, as well as achieve 10,000 queries per second.

## Comparisons to Node.js
My HTTP server is closely modelled after the event-driven architecture that Node.js uses. For my HTTP server, I used `epoll` to wait on a read/write event that can be performed on sockets matched to a specific client. 

Meanwhile, Node.js employs `libuv` behind the scenes to manage its events, which ultimately makes use of an event demultiplexer like `epoll`. On top of waiting on IO events, it also waits on other events like timeouts and immediate callbacks.

## Design decisions
There were several ways to implement a server that can handle thousands of simultaneous connections. Below, I list my understanding of the different approaches based on the research I have done. The examples below are based on the assumption of having 1000 simultaenous connections.

<table>
  <tbody>
    <tr>
      <th text-align="center">No.</th>
      <th text-align="center">Type of polling</th>
      <th text-align="center">Type of IO</th>
      <th>Issues</th>
    </tr>
    <tr>
      <td align="center">1</td>
      <td align="center">No polling</td>
      <td align="center">Blocking</td>
      <td>
        <ul>
          <li>On a single-thread, reading on an empty socket will cause a block. This means that we are unable to check on the other 999 sockets until the first socket gets unblocked.</li>
          <li>The alternative is to spin up 1000 threads which is super expensive when all we are doing is waiting, reading and writing.</li>
        </ul></td>
    </tr>
    <tr>
      <td align="center">2</td>
      <td align="center">No polling</td>
      <td align="center">Non-blocking</td>
      <td>
        <ul>
          <li>Now that IO is non-blocking, we can continuously loop through all 1000 sockets and try to read/write. However, if there is no read/write happening, the loop will result in busy waiting which consumes unnecessary CPU.</li>
        </ul></td>
    </tr>
    <tr>
      <td align="center">3</td>
      <td align="center">Level-triggered epoll</td>
      <td align="center">Blocking</td>
      <td>
        <ul>
          <li>Instead of looping through sockets one at a time, we can register interest for the sockets that we are interested in using <code>epoll</code>.       
          When data is available for reading, <code>epoll_wait</code> unblocks. However, if we are unable to finish reading due to the limited buffer, we will have to loop back and call <code>epoll_wait</code> again, which results in a lot of syscalls.</li>
          <li>While we can reduce the number of syscalls by repeatedly reading on the socket until everything has been read, we will not know when we have finished reading the socket. Thus, the next read will be blocking which defeats the purpose of polling.</li>
          <li>Meanwhile, unless the write buffer is full, write is always available and the <code>EPOLLOUT</code> event will always be emitted. This results in <code>epoll_wait</code> always returning, leading to busy waiting as well.</li>
        </ul></td>
    </tr>
    <tr>
      <td align="center">4</td>
      <td align="center">Level-triggered epoll</td>
      <td align="center">Non-blocking</td>
      <td>
        <ul>
          <li>With non-blocking IO, we can now repeatedly read without the fear of the last blocking read. However, we will still have busy waiting from epoll waking up for write events.</li>
        </ul></td>
    </tr>                                          
    <tr>
      <td align="center">5</td>
      <td align="center">Edge-triggered polling</td>
      <td align="center">Blocking</td>
      <td>
        <ul>
          <li>We no longer have the issue of busy waiting because the edge-triggered epoll loop only wakes up when there is a new event. However, suppose the client sends 2Kb worth of data and we are only able to read 1Kb. We will then loop back to <code>epoll_wait</code> where it will block. Meanwhile, the client might be waiting for a response and blocks as well. This results in a deadlock.</li>
          <li>If there is more data to write than to the write buffer can handle, it will block on the write, which once again defeats the purpose of polling.</li>
        </ul></td>
    </tr>  
    <tr>
      <td align="center">6</td>
      <td align="center">Edge-triggered polling</td>
      <td align="center">Non-blocking</td>
      <td>
        <ul>
          <li>The deadlock issue remains, but having non-blocking IO solves the issue of blocking on write.</li>
        </ul></td>
    </tr>         
    <tr>
      <td align="center">7</td>
      <td align="center">Edge-triggered polling</td>
      <td align="center">Non-blocking + looping on read/write till <code>EGAIN</code></td>
      <td>
        <ul>
          <li>To solve the deadlock issue, we repeatedly read until <code>EGAIN</code>, which means all the data has been read. For write, we write till <code>EGAIN</code>. We then loop back to <code>epoll_wait</code> and wait. Once the buffer empties out, <code>EPOLLOUT</code> will be triggered and we can write once more.</li>
        </ul></td>
    </tr>      
  </tbody>
</table>

The ideal approach is the last, which is also described in the epoll man page. However, since the project deals with small amounts of data, I decided that repeatedly looping for <code>EGAIN</code> adds to unnecessary complexity. Thus, I opted for the 6th approach.

## Technology

- C++
- Linux

## Resources
These are the resources that I used to better understand Node.js' event-driven model, edge-triggered vs level-triggered epoll, and non-blocking IO.
- [Node.js event loop and event queues](https://blog.insiderattack.net/event-loop-and-the-big-picture-nodejs-event-loop-part-1-1cb67a182810)
- [Event demultiplexing and event loop](https://subscription.packtpub.com/book/programming/9781783287314/1/ch01lvl1sec09/the-reactor-pattern)

- [Epoll man page](https://man7.org/linux/man-pages/man7/epoll.7.html)
- [Select vs Poll vs Epoll](https://hechao.li/2022/01/04/select-vs-poll-vs-epoll/)
- [When EPOLLIN and EPOLLOUT is triggered for EPOLLET](https://stackoverflow.com/a/12897334)
- [Utility of edge-triggered over level-triggered epoll](https://stackoverflow.com/a/51757553)

- [Blocking I/O, Non-blocking I/O and Epoll](https://eklitzke.org/blocking-io-nonblocking-io-and-epoll)
- [Example code of non-blocking I/O and Epoll](https://github.com/eklitzke/epollet/blob/master/poll.c)

## Contact

Created by [Jia Xian](https://www.linkedin.com/in/tanjiaxian99/)
