# MY_STDIN  
  
### About  
This is a project meant to help understand the way the standard input / output library works in C. It includes implementations of basic file IO functionalities as well as processes which execute a given command and have their standard input / output redirected to a pipe for communication with the main process.  
  
### How to use it  
* Download or clone the repository
* compile the library using the Makefile provided, from the root directory of the project, which creates a static library called libmy_stdio.a  
`make`
* link the library to your source file  
`gcc <source file(s)> -o <executable name> -lmy_stdin -L<path to project root directory>`

Read > fseek > write  
write > fseek / fflush > fread
