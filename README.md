# CTAS
A Compile Time Application Server for C++


## Goal
This project aims to provide an easy to use HTTP webserver with lots of compile time type checking.

## Requirements
- C++ 17
- Until C++ standardizes a networking library Christian Kohlhoff's Networking TS prototype is used
- For lots of the compile time checking this project uses concept-like template checking via std::enable_if.

Once networking and concepts are in the C++ standard, this project doesn't have any external dependencies.

## How to
### Pages
Create a class that fullfills the requirements (concept) of a page and register it at the server with a certain path. The "HandleRequest" method is called once an external http request requests the given path.

### Sessions
A session is also a user defined class, that is attached to a request before it is delivered to the page.

### Session Provider
Gives you the opportunity to control how sessions are stored. Hashmaps, files or database are a few suggestions.
