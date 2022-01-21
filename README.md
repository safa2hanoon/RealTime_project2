#RealTime_project2
Palestine => Jordan crossing border management
We would like to create a multi-processing application that simulates the daily operation
of the crossing border from Palestine to Jordan. The case can be described as follows:
• Passengers arrive randomly at the crossing border. Most of them are Palestinians
and Jordanians. However, some of them might be foreigners. The percentage of
Palestinian, Jordanian or foreign passengers should be user-defined.
• To cross the border, passengers need to have their papers checked first at crossing
points by Palestinian officers. Most of these crossing points are for Palestinian and
Jordanian passengers only while the remaining ones are only for foreigners.
• If passengers have their passports expired or do not have them (damn it!), they are
denied crossing.
• The officers at the crossing points do not process passengers at the same rate. Pas-
sengers who wait a lot at these crossing points get impatient and might decide to
return home if they are not handled within a time limit.
• Every time P passengers have crossed the crossing points, they are grouped in a
hall waiting for buses to transfer them to the Jordanian side. Each bus can transfer
a maximum number of passengers per trip (e.g. 50). The maximum number of
passengers that buses can transfer in each trip should be user-defined.
• When the number of passengers in the hall has exceeded a certain threshold Hmax,
the crossing points stop handling passengers until the number of passengers in the
hall drops below a threshold Hmin. In the meantime, passengers continue to add to
the queues of the crossing points.
• Assume that there are B buses that will handle passengers’ transfer to the Jordanian
side. Once a bus leaves the Palestinian side to the Jordanian side, it becomes
unavailable for Tbi seconds before it comes back to the Palestinian side.
• Assume that the passengers get out of our system once they transferred to the
Jordanian side.
• The simulation should end if any of the following is true:
– More than Pg passengers have been granted access at crossing points.
– More than Pd passengers have been denied access at crossing points.
– More than Pu passengers got too impatient and decide to cancel their trip to
Jordan and return home.
![image](https://user-images.githubusercontent.com/58912796/150557085-20278df3-9713-4261-87c2-9d02585f91b3.png)
What you should do
• Write the code for the above-described application using a multi-processing ap-
proach.
• Check that your program is bug-free. Use the gdb debugger in case you are having
problems during writing the code (and most probably you will :-). In such a case,
compile your code using the -g option of the gcc.
• In order to avoid hard-coding values in your programs, think of creating a text file
that contains all the values that should be user-defined and give the file name as an
argument to the main program. That will spare you from having to change your
code permanently and re-compile. As a preliminary list, consider putting the fields
described above (e.g. P, O, B, etc). Use and complete the following list:
NUMBER CROSSING POINTS P 5
NUMBER CROSSING POINTS J 3
NUMBER CROSSING POINTS F 1
NUMBER OFFICERS 9
NUMBER BUSES 4
BUS SLEEP PERIOD 1 10
The last line means that buses sleeping period will be in the interval of 1 - 10
seconds.
• If you are familiar with building graphical primitives under Linux, it will be great if
you can do it to simulate the behavior of the border visually. The primitives can be
as simple as boxes for passengers, triangles (for officers), circles (for buses) and the
like. Otherwise, you need to use lots of printf!
