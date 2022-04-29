# MY_STDIN  
  
### About  
This is a project meant to help understand the way the standard input / output library works in C. It includes implementations of basic file IO functionalities as well as processes which execute a given command and have their standard input / output redirected to a pipe for communication with the main process.  
  
### Built with
* C
* POSIX system calls
  
### How to use it  
* Download or clone the repository
* Include the my_stdin.h header in your source file
* compile the library using the Makefile provided, from the root directory of the project, which creates a static library called libmy_stdio.a  
`make`
* link the library to your source file. Example:  
`gcc <source file(s)> -o <executable name> -lmy_stdin -L<path to project root directory>`

### Notes
* This project is meant to be used on linux systems
* According to the POSIX standard, the library assumes the user will include a seek - `my_fseek` - between a read - `my_fread` or `fgetc` - followed by a write - `my_fwrite` or `my_fputc` - call.
* According to the POSIX standard, the library assumes the user will include a seek or fflush - `my_fseek` or `my_fflush` - between a write - `my_fwrite` or `my_fputc` - followed by a read - `my_fread` or `fgetc` - call.
* If the user does not adhere to the standard, the behaviour of the library will likely not be the one intended
