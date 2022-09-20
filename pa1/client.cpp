/*
	Original author of the starter code
    Tanzir Ahmed
    Department of Computer Science & Engineering
    Texas A&M University
    Date: 2/8/20
	
	Please include your Name, UIN, and the date below
	Name: Chase Albright
	UIN: 529008060
	Date: 9/6/22
*/
#include "common.h"
#include "FIFORequestChannel.h"
#include <iostream>
#include <fstream>
#include<chrono>

using namespace std;
using namespace std::chrono;


int main (int argc, char *argv[]) {
	int opt;
	int p = -1;
	double t = -1.0;
	int e = -1;
	int buffm = MAX_MESSAGE;
	bool new_chan = false;
	string buffhold = "256";
	string filename = "";
	bool check = false;
	vector<FIFORequestChannel*> channels;
	while ((opt = getopt(argc, argv, "p:t:e:f:m:c")) != -1) {
		switch (opt) {
			case 'p':
				p = atoi (optarg);
				break;
			case 't':
				t = atof (optarg);
				break;
			case 'e':
				e = atoi (optarg);
				break;
			case 'f':
				filename = optarg;
				break;
			case 'm':
				buffhold = optarg;
				buffm = atoi(optarg);
				break;
			case 'c':
				new_chan = true;
				break;
		}
	}

	//give arguements for server
	//server needs './server','-m', '<val for -m arg>', 'NULL'
	
	char* const cmd1[] = {(char*) "./server", (char*) "-m", (char*) buffhold.c_str(), NULL};

	//fork
	if(!fork())
	{
		cout<<"child running"<<endl;
		//in child, run execvp using server arguements
		execvp(cmd1[0],cmd1);
	}
	//client
	else{
	
	sleep(1);

	cout<<"run parent"<< endl;


    FIFORequestChannel cont_chan("control", FIFORequestChannel::CLIENT_SIDE);
	channels.push_back(&cont_chan);
	//making new channel
	if(new_chan){
		// send newchannel request to server
		cout << " enter your message :): "<<endl;
		MESSAGE_TYPE nc = NEWCHANNEL_MSG;
		cont_chan.cwrite(&nc, sizeof(MESSAGE_TYPE));
		 
		//variable to hold name
		byte_t buffpipe[MAX_MESSAGE];
		memset(buffpipe,0,MAX_MESSAGE);
		//cread response
		cont_chan.cread(buffpipe,MAX_MESSAGE);
		//call fifo requestchannel constructor with the name from the server
		FIFORequestChannel* windpipe = new FIFORequestChannel(buffpipe, FIFORequestChannel::CLIENT_SIDE);
		memset(buffpipe,0,MAX_MESSAGE);
		cout << "channel established"<<endl;
		/*
		MESSAGE_TYPE q = QUIT_MSG;
		windpipe.cwrite(&q,sizeof(MESSAGE_TYPE));
		*/
		//push the new channel into the vector
		channels.push_back(windpipe);
		check = true;
		
	}
	//FIFORequestChannel chan = cont_chan;
	FIFORequestChannel chan = *(channels.back());

	//singe datapoint, only run p,t,e !=-1
	if((p != -1 && t!= -1.0 && e!= -1))
	{
		auto start = high_resolution_clock::now();

		// example data point request
		char buf[MAX_MESSAGE]; // 256
		memset(buf,0,MAX_MESSAGE);
		datamsg x(p, t, e); //change from hardcoding to user values
		
		memcpy(buf, &x, sizeof(datamsg));
		chan.cwrite(buf, sizeof(datamsg)); // question
		double reply;
		chan.cread(&reply, sizeof(double)); //answer
		cout << "For person " << p << ", at time " << t << ", the value of ecg " << e << " is " << reply << endl;
		auto stop = high_resolution_clock::now();
		auto duration = duration_cast<milliseconds>(stop-start);
		cout<<"time taken: "<<duration.count()<<" miliseconds with buffersize: " <<buffm << endl;

	}

	//else if p!=-1, request 1000 datapoint
	else{
	if(p!=-1.0)
	{
		auto start = high_resolution_clock::now();


		ofstream writer;
		cout<<"writing stuff"<<endl;
		writer.open("received/x1.csv");
		//byte_t buffpuff[MAX_MESSAGE];
		//memset(buffpuff,0,MAX_MESSAGE);
		//loop first 1000 lines
		for(int i=0; i<1000; i++)
		{
			double time = double(i*.004);
			//send request for ecg 1
			datamsg ecg1(p,time,1);
			//cout<<"got val"<<endl;
			//server
			chan.cwrite(&ecg1,sizeof(datamsg));
			double value1;
			//back to client
			chan.cread(&value1,sizeof(double));

			//ecg2
			datamsg ecg2(p,time,2);
			//server
			chan.cwrite(&ecg2,sizeof(datamsg));
			double value2;
			//back to client
			chan.cread(&value2,sizeof(double));

			//write line to received/x1.csv
			writer<<time<<","<<value1<<","<<value2<<endl;
			
		}
		writer.close();
		auto stop = high_resolution_clock::now();
		auto duration = duration_cast<milliseconds>(stop-start);
		cout<<"time taken: "<<duration.count()<<" miliseconds with buffersize: " <<buffm << endl;
	}
	}
    //sending a non-sense message, you need to change this
	if(filename != "")
	{
	auto start = high_resolution_clock::now();

	string fname = filename;
	cout<<"file name : " << filename<<endl;
	filemsg fm(0, 0);
	//string fname = "teslkansdlkjflasjdf.dat";

	//packet into buf2
	int len = sizeof(filemsg) + (fname.size() + 1);
	char* buf2 = new char[len];
	memcpy(buf2, &fm, sizeof(filemsg));
	//move pointer forward
	strcpy(buf2 + sizeof(filemsg), fname.c_str());

	chan.cwrite(buf2, len);  // I want the file length;
	//cout<<sizeof(buf2)<<endl; 8

	int64_t filesize = 0;
	chan.cread(&filesize, sizeof(int64_t));

	cout<<"filesize "<<filesize<<endl;

	//loop over segments in the file fileze/buffer cap
	int loops = ceil((double) filesize/buffm);;
	cout<<loops<<endl;

	std::ofstream wri;
	wri.open("received/" + fname, ios_base::binary);
	//store biggest reponse which is buff capacity/char* buf3 = sizeof(buffm); //create buffer of buffer capacity(m);
	char * buf3 = new char[buffm];
	cout<<"len is " << len <<endl;
	cout<<"buff2: " << sizeof(buf2)<<endl;
	cout<<"filemsg: " << sizeof(filemsg)<<endl;
	//delete[] buf2;

	for(int i=0;i<loops; i++)
	{
		
		filemsg* file_req = (filemsg*) buf2;
		memset(buf3,0,buffm);
		
	if(i==(loops-1))
	{	
		file_req->offset = i*buffm;
		file_req->length = filesize-(file_req->offset);
	
		
	
		filemsg file_req(i*buffm,filesize-(i*buffm));

		int len = sizeof(filemsg) + (fname.size() + 1);
		/*
		char* buf2 = new char[len];
		memcpy(buf2, &file_req, sizeof(filemsg));
		strcpy(buf2 + sizeof(filemsg), fname.c_str());
		*/
		chan.cwrite(buf2, len);  // I want the file length;
		//int64_t filesize = 0;
		chan.cread(buf3, filesize-(i*buffm));
		// write buff 3 into file
		wri.write(buf3, filesize-(i*buffm));
		//delete[] buf2;
	}
	else 
	{

		
		file_req->offset = i*buffm;
		file_req->length = buffm;
		
		//filemsg file_req(i*buffm,buffm);
		//create filemsg instance
		//filemsg* file_req = (filemsg*) buf2;
		//file_req->offset = 
		//file_req->length =
		//send request (buf2)
		//chan.cwrite(buf2, len);
		//receive the response
		//cread into buf3 length file_req->len
		//write buff3 into file
		int len = sizeof(filemsg) + (fname.size() + 1);
		//cout<<file_req->length<<endl;
		//cout<<offset <<endl; 30
		/*
		cout<<len<<endl;
		cout<<file_req->offset<<endl;
		cout<<file_req->length<<endl;
		*/
		//move pointer forward
		//char* buf1 = new char[len];
		/*
		char* buf2 = new char[len];
		memcpy(buf2, &file_req, sizeof(filemsg));
		strcpy(buf2 + sizeof(filemsg), fname.c_str());
		*/

		chan.cwrite(buf2, len);  // I want the file length;
		
		chan.cread(buf3, buffm);
	
		// write buff 3 into file
		wri.write(buf3, buffm);
		//cout<<"filesize"<<filesize<<endl;
		//cout<<file_req->offset<<endl;
		//delete[] buf2;
	}

	}
	wri.close();
	delete[] buf2;
	delete[] buf3;
	auto stop = high_resolution_clock::now();
	auto duration = duration_cast<milliseconds>(stop-start);
	cout<<"time taken: "<<duration.count()<<" miliseconds with buffersize: " <<buffm << endl;
	//close and delete channel if needed
	}


	if(check)
	{
		//close and deletes/
		//delete *chan;
		MESSAGE_TYPE m = QUIT_MSG;
		chan.cwrite(&m, sizeof(MESSAGE_TYPE));
		delete &chan;


	}
	// closing the channel 
    //MESSAGE_TYPE m = QUIT_MSG;
    //chan.cwrite(&m, sizeof(MESSAGE_TYPE));
	MESSAGE_TYPE m = QUIT_MSG;
	cont_chan.cwrite(&m, sizeof(MESSAGE_TYPE));
	//delete &chan;

	//edge cases
	//file type wrong/
	//file size 0/
	//file bufer bigger than size/good
}
}

