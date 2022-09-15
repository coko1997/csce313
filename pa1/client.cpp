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

using namespace std;



double writePatientPoint(FIFORequestChannel *channel, int p, double s, int e){

	// make a data message packety and send it to server
	datamsg packet(p,s,e);
	channel->cwrite(&packet, sizeof(datamsg));


	// we want to read response from server and then return the response
	double res;
	channel->cread(&res, sizeof(res));

	return res;
}

void writePatientData(FIFORequestChannel *channel, int p){
	ofstream fout;
	fout.open("received/x1.csv");
	double ecg1;
	double ecg2;
	double t;
	
	for (int i = 0; i < 1000; i++){
		t = (0.004 * i);
		if (t > 59.996){
			break;
		}
		ecg1 = writePatientPoint(channel,p,t,1);
		ecg2 = writePatientPoint(channel,p,t,2);
		fout << t << "," << ecg1 << "," << ecg2 << std::endl;
	}
	fout.close();
}




int main (int argc, char *argv[]) {
	int opt;
	int p = -1;
	double t = -1.0;
	int e = -1;
	int m = MAX_MESSAGE;
	bool new_chan = false;

	vector<FIFORequestChannel*> channels;
	
	string filename = "";
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
				m = atoi(optarg);
				break;
			case 'c':
				new_chan = true;
				break;
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	pid_t pid = fork();

	if (pid < 0){
		std::cerr << "failed" << std::endl;
		return 1;
	} else if (pid == 0){

		char * args[] = {const_cast<char *>("./server"),const_cast<char *>("-m"), const_cast<char *>(to_string(m).c_str()), nullptr};
		if(execvp(args[0], args) < 0){ // nothing below executes
			perror("execvp");
			exit(EXIT_FAILURE);
		}

	} else {

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		FIFORequestChannel cont_chan("control", FIFORequestChannel::CLIENT_SIDE); // dont forget to delete
		channels.push_back(&cont_chan);


		if (new_chan){ // logic for a new channel

			MESSAGE_TYPE nc = NEWCHANNEL_MSG;

			cont_chan.cwrite(&nc, sizeof(MESSAGE_TYPE)); 

			char channel[100]; // arbitrary length for name

			cont_chan.cread(channel, sizeof(channel));

			FIFORequestChannel * chan = new FIFORequestChannel(channel, FIFORequestChannel::CLIENT_SIDE);

			channels.push_back(chan);
			

		}



		FIFORequestChannel chan = *(channels.back());


		if (p != -1 && t != -1 && e != -1){

			// example data point request
			char buf[MAX_MESSAGE]; // 256 // set to defined capacity
			datamsg x(p,t,e);
			memcpy(buf, &x, sizeof(datamsg));
			chan.cwrite(buf, sizeof(datamsg)); // question
			double reply;
			chan.cread(&reply, sizeof(double)); //answer
			cout << "For person " << p << ", at time " << t << ", the value of ecg " << e << " is " << reply << endl;




		} else if (p != -1) {

			writePatientData(&chan, p);

		} else if (filename != ""){ // condition for sending a file

			filemsg fm(0, 0);


			int len = sizeof(filemsg) + (filename.size() + 1); // size of packet

			char* buf2 = new char[len];
			memcpy(buf2, &fm, sizeof(filemsg));
			strcpy(buf2 + sizeof(filemsg), filename.c_str());
			
			chan.cwrite(buf2, len);  // I want the file length;

			__int64_t filesize = 0;
			chan.cread(&filesize, sizeof(__int64_t));

			char* buf3 = new char[m]; // buffer capacity or ret_buf


			// number of messages to be sent
			int packet_count = (filesize / m) ; // ex. 1000 / 256 = 3 -> 4 total messages


			//loop
			filemsg * file_req = (filemsg*) buf2;


			// open a file to write to
			string dir = "received/";
			string filepath = dir + filename;
			FILE* outfile = fopen(filepath.c_str(), "wb"); // read and write new file

			// set base cases
			file_req->length = m;
			file_req->offset = 0;

			// loop
			for (int i = 0; i < packet_count; i++){ // handles first n-1 segments or all segments if perfectly divisible
				
				//read channel and write file binary
				chan.cwrite(buf2, len);
				chan.cread(buf3, file_req->length);
				fwrite(buf3, 1, file_req->length, outfile);

				//update len and offset
				file_req->offset += m;
			}

			// handle the left over segment
			if ((filesize - (packet_count * m)) != 0){ //

				file_req->length = filesize - (packet_count * m);

				delete[] buf3;
				buf3 = new char[filesize - (packet_count * m)];
				chan.cwrite(buf2, len);
				chan.cread(buf3, file_req->length);
				fwrite(buf3, 1, file_req->length, outfile);

			}

			fclose(outfile);

			delete[] buf2;
			delete[] buf3;


		}

		
		
		
//////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////////
		/*
		*
		* closing a the channels and server termination
		*
		*/
		int count = 0;
		for (auto c : channels){ // close and delete everything but control channel
			if (count != 0){
				MESSAGE_TYPE m = QUIT_MSG;
				c->cwrite(&m, sizeof(MESSAGE_TYPE));
				delete c;
			}
			count++;
		}

		// close the control channel
		MESSAGE_TYPE m = QUIT_MSG;
		cont_chan.cwrite(&m, sizeof(MESSAGE_TYPE));

		// wait for server to close and terminate program
		waitpid(pid, nullptr, 0);


	}

}

