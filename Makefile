FLAGS = -g -Wall -std=c++11

all: r1 r2 r3 clean

r1: RM1.cpp
	g++ ${FLAGS} -o rm1 RM1.cpp -pthread -Wl,--no-as-needed

r2: RM2.cpp
	g++ ${FLAGS} -o rm2 RM2.cpp -pthread -Wl,--no-as-needed

r3: RM3.cpp
	g++ ${FLAGS} -o rm3 RM3.cpp -pthread -Wl,--no-as-needed

clean:
	if [ -d "/var/www/html/1" ];then	\
		rm -r /var/www/html/1;			\
	fi;
	if [ -d "/var/www/html/2" ];then	\
		rm -r /var/www/html/2;			\
	fi;
	if [ -d "/var/www/html/3" ];then	\
		rm -r /var/www/html/3;			\
	fi;



#all: texty clean

#texty: texty.cpp
#	g++ ${FLAGS} -o texty texty.cpp -pthread -Wl,--no-as-needed

#clean:
	# if [ -d "/var/www/html/files" ];then	\
	# 	rm -r /var/www/html/files;			
	# fi




# all: 0.bowing 1.bowing 1b.bowing 2.bowing 3.bowing 4.bowing 4b.bowing 5.bowing 6.bowing 7.bowing 8.bowing 9.bowing


# 0.bowing: 0.bowing.cpp
#	g++ ${FLAGS} -o 0.bowing 0.bowing.cpp

#1.bowing: 1.bowing.cpp
#	g++ ${FLAGS} -o 1.bowing 1.bowing.cpp -pthread

#1b.bowing: 1b.bowing.cpp
#	g++ ${FLAGS} -o 1b.bowing 1b.bowing.cpp -pthread

#2.bowing: 2.bowing.cpp
#	g++ ${FLAGS} -o 2.bowing 2.bowing.cpp -pthread

#3.bowing: 3.bowing.cpp
#	g++ ${FLAGS} -o 3.bowing 3.bowing.cpp -pthread

#4.bowing: 4.bowing.cpp
#	g++ ${FLAGS} -o 4.bowing 4.bowing.cpp -pthread

#4b.bowing: 4b.bowing.cpp
#	g++ ${FLAGS} -o 4b.bowing 4b.bowing.cpp -pthread

#5.bowing: 5.bowing.cpp
#	g++ ${FLAGS} -o 5.bowing 5.bowing.cpp -pthread

#6.bowing: 6.bowing.cpp
#	g++ ${FLAGS} -o 6.bowing 6.bowing.cpp -pthread

#7.bowing: 7.bowing.cpp
#	g++ ${FLAGS} -o 7.bowing 7.bowing.cpp -pthread

#8.bowing: 8.bowing.cpp
#	g++ ${FLAGS} -o 8.bowing 8.bowing.cpp -pthread

#9.bowing: 9.bowing.cpp
#	g++ ${FLAGS} -o 9.bowing 9.bowing.cpp -pthread

#clean:
#	rm -f 0.bowing 1.bowing 1b.bowing 2.bowing 3.bowing 4.bowing 4b.bowing 5.bowing 6.bowing 7.bowing 8.bowing 9.bowing

