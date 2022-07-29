# Consumer-Producer-C
A Consumer Producer project with C, for my computer systems master's classes.


we have two versions of the same program:

the first one, called "projeto comum" is the project withou any modifications for the Computer Systems classes.

To start the project, it is only necessary to click on the Project file and, it can be seen that it will generate an output, following the expected pattern, if you want to add any matrix, it will be necessary to create a new file as specified in the work, each file must contain two 10x10 matrices one under the other.
To run the project it is necessary to execute the command “gcc projeto.c -o projeto” followed by “./projeto”

The second one, called "projeto extra" contains modifications, to solve the same problems, but with a better performance methods

To start the project, it is only necessary to click on the Project file and, it can be seen that it will generate an output, following the expected pattern, if you want to add any matrix, it will be necessary to create a new file as specified in the work, each file must contain two 10x10 matrices one under the other.
Here, in this case, it is also expected to run the command to run the daemon:

“gcc projeto.c -o projetoDaemon -lpthread -fopenmp”

For the daemon project to work, it is necessary to create change the path of the input.in file for the input of the arrays since the daemon operates with absolute paths.
After these adjustments, we just need to run “./projetoDaemon $PWD/entrada.in $PWD/saida.out”

Optionally, you can run the command below to create an executable and run the project:
“gcc -Wall -g -o projetoExec projeto.c -lpthread -fopenmp”
to run run, write: “./projetoExec”
To view the logs, just type the command:
“grep Logs /var/log/syslog”


obs: The program was written and tested on Linux
