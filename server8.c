#include <stdint.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h> 
#include <string.h>
#include <stdio.h> 
#include <stdlib.h> 
#include <fcntl.h> 
#include <sys/ioctl.h> 
#include <linux/spi/spidev.h> 
#include <chrono>
#include <thread>
#include <iostream>
#include <vector>
#include <fstream>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <mutex>
#include <condition_variable>
#include <Python.h>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <termios.h>

using namespace std;
std::mutex mtx;   

#define SPI_DEVICE_PATH '/dev/spidev1.0'
const string gpio_path ="/sys/class/gpio/";
const int gpio_num=48;
const int HRST=49;
const int MRST=60;
vector<int> gpio_nums={48,115,20,112};//48-DONE/115-READY/20-ERROR/112-WDERR

void gpio_export(int gpio_num){
    fstream fs(gpio_path + "export",fstream::out);
    fs << gpio_num;
    fs.close();
}
void gpio_set_direction(int gpio_num,string direction){
    fstream fs(gpio_path + "gpio"+to_string(gpio_num)+"/direction",fstream::out);
    fs << direction;
    fs.close();
}

char gpio_read_valu(int gpio_num){
    fstream fs(gpio_path + "gpio"+to_string(gpio_num)+"/value",fstream::in);
    char value;
    fs >> value;
    fs.close();
    return value;
}

void gpio_write_value(int gpio_num,char value){
    fstream fs(gpio_path + "gpio"+to_string(gpio_num)+"/value",fstream::out);
    fs << value;
    fs.close();

}

vector<vector<char> > splitArray(vector<char> arr, int size) {
    vector<vector<char> > result;
    int len = arr.size();
    for (int i = 0; i < len; i += size) {
        vector<char> temp;
        for (int j = i; j < i + size && j < len; j++) {
            temp.push_back(arr[j]);
        }
        result.push_back(temp);
    }
    return result;
}

vector<string> splitArray1(string arr, int size) {
    vector<string> result;
    int len = arr.size();
    for (int i = 0; i < len; i += size) {
        string temp;
        for (int j = i; j < i + size && j < len; j++) {
            temp.push_back(arr[j]);
        }
        result.push_back(temp);
    }
    return result;
}


string CHK(string sr){
    int result = 0;
    int result1;
    for (char c:sr){
        result ^= c;
    }
    if (result==0){
        result1 = 255;
    }
    else{
        result1 = result-1;
    }
    stringstream ss;
    ss << setw(5) << setfill('0') << result1; 
    string str = ss.str();
    return str;
}

char* get_time(){
    time_t now = time(0);
    char* dt = ctime(&now);
    return dt;
}

//Command test
void commands_test(string filename){
     string qt;
    string line;
    vector<string> lines;
    // open file
    ifstream file("/home/debian/bin/"+filename);
    if (file.is_open());      
        {
            while(getline(file,line))
            {
                 lines.push_back(line);
            }
        }
    // close file
    file.close();
    std::ofstream files("961ch1.log", std::ios::out | std::ios::trunc); 
    files.close();
    char* tm;
    for(int r = 0;r < lines.size();r++)
    {
        tm = get_time();
    ofstream file3("961ch1.log",ios::app);
    if (file3.is_open());
        {
        file3 << tm+'\n' << std::endl;
        }
    file3.close();

    ofstream file2("961ch1.log",ios::app);
    if (file2.is_open());
        {
            std::string lin;
        for (const auto& c : lines[r]) {
            if (c == '\n') {
                file2 << lin << std::endl;
                lin.clear();
            } else {
                lin += c;
            }
        }
        file2 << lin+'\n' << std::endl;
        }
    file2.close();
        
    qt=CHK(lines[r]+":CHK=");
    lines[r]='\x01'+lines[r]+":CHK="+qt+'\x04';
    vector<string> result;
    int size = 4096;
    result = splitArray1(lines[r], size);
    int num = result.size();
  
    int fd;
	int status;
	int m1;
	int m2;
	int m3;
    const char *device = "/dev/spidev1.0";
    // Open the device file
    fd = open(device, O_RDWR);
    if (fd < 0) {
        perror("Failed to open SPI device");
        exit(1);
    }
    // Configure the SPI interface
    uint8_t mode = SPI_MODE_3;
    uint32_t speed = 24000000; // 24MHz
    uint8_t bits_per_word=8; 
	m1=ioctl(fd, SPI_IOC_WR_MODE, &mode);
	m2=ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
	m3=ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits_per_word);
    if ( m1< 0) {
        perror("Failed to set SPI mode");
        exit(1);
    }
    if ( m2< 0) {
        perror("Failed to set SPI speed");
        exit(1);
    }
	if ( m3< 0) {
        perror("Failed to set SPI speed");
        exit(1);
    }

char value0;
gpio_export(gpio_num);
gpio_set_direction(gpio_num,"in");
value0 = gpio_read_valu(gpio_num);
for(int k = 0;k < num;k++)
{
    uint8_t data[result[k].size()];
    int i = 0;
    for (int j=0;j<result[k].size();j++)
    {
        data[i++]=(int)result[k][j];
    }
    uint8_t rx[sizeof(data)];

    char value;
while(true){
    // Read the status of the GPIO pin
    value = gpio_read_valu(gpio_num);
    if(value=='1'){break;}
    }
    struct spi_ioc_transfer tr1={
    .tx_buf = (unsigned long)data,
    .rx_buf = (unsigned long)rx,
    .len = sizeof(data),
    .speed_hz = speed,
    .delay_usecs = 0,
    .bits_per_word = 8,
    .cs_change = 0,
    };
    // Send data
	status = ioctl(fd, SPI_IOC_MESSAGE(1), &tr1);
    if (status<0) {
        perror("Failed to send SPI message2");
        exit(1);
    }
} 
uint8_t tx2[4096] = {0};
uint8_t rx2[4096];
char value2;
vector<uint8_t> list;
while(true)
{
    while(true)
{
    // Read the status of the GPIO pin
    value2=gpio_read_valu(gpio_num);
    if(value2=='1'){break;}
 } 
    struct spi_ioc_transfer tr3={
    .tx_buf = (unsigned long)tx2,
    .rx_buf = (unsigned long)rx2,
    .len = sizeof(tx2),
    .speed_hz = speed,
    .delay_usecs = 0,
    .bits_per_word = 8,
    .cs_change = 0,
    };
    // send data
	status=ioctl(fd, SPI_IOC_MESSAGE(1), &tr3);
    if (status<0) {
        perror("Failed to send SPI message2");
        exit(1);
    }
    uint8_t* pRx2 = rx2; 
    list.insert(list.end(), pRx2, pRx2 + sizeof(rx2) / sizeof(uint8_t));
    auto iter = std::find(rx2, rx2 + sizeof(rx2), '\x04');
    if (iter != rx2 + sizeof(rx2)){break;}
 }

    list.erase(list.begin());
    auto rit=find(list.rbegin(),list.rend(),'=');
    int index=distance(rit,list.rend());
    list.erase(list.begin()+index-4,list.end());
    char* tm0;
    tm0 = get_time();
    ofstream file0("961ch1.log",ios::app);
    if (file0.is_open());
        {
        file0 << tm0+'\n' << std::endl;
        }
    file0.close();
   
    ofstream file1("961ch1.log",ios::app);
    if (file1.is_open());
        {
        std::string lin;
        for (const auto& c : list) {
            if (c == '\n') {
                file1 << lin << std::endl;
                lin.clear();
            } else {
                lin += c;
            }
        }
        file1 << lin+'\n' << std::endl;
        }
    file1.close();
    close(fd);
}
}

//Serial communication
string UART_connection(string command) {
    // Open the serial device file
    const char* port = "/dev/ttyO4"; // The serial device file path
    int fd = open(port, O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd < 0) {
        perror("Open serial port failed");
    }
    // Configure serial port parameters
    struct termios options;
    tcgetattr(fd, &options);
    cfmakeraw(&options); // Set to raw mode
    cfsetispeed(&options, B115200); // Sets the input baud rate
    cfsetospeed(&options, B115200); // Sets the output baud rate
    options.c_cflag |= CLOCAL | CREAD; // Enable receiver and local mode
    options.c_cflag &= ~CSIZE; // Masked character length bits
    options.c_cflag &= ~CRTSCTS; // Disable hardware flow control
    options.c_cflag |= CS8; // Set the data bits to 8 bits
    options.c_cflag &= ~PARENB; // Disable the check digit
    options.c_cflag &= ~CSTOPB; // Set the stop bit to 1 bit
    tcsetattr(fd, TCSANOW, &options);

    // send datas
    const char* data = command.c_str(); 
    int len = strlen(data);
    int nwrite = write(fd, data, len);
    cout<<nwrite<<endl;
    if (nwrite < 0) {
        perror("Write failed");
    }
    int flags = fcntl(fd, F_GETFL, 0);
    flags |= O_NONBLOCK;
    fcntl(fd, F_SETFL, flags);
    // receive datas
    int nread = 0;
    vector<char> buff;
    while (true) {
        char buf[1024];
        nread = read(fd, buf, sizeof(buf));
        if (nread>0)
        {
            for (int j = 0; j < nread; j++)
            {
            buff.insert(buff.end(), buf[j]);
            }       
            if (buf[nread-1]=='\x04'){ break;}   
        }
        else{
        if (nread < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                usleep(100000);
            } else {
                perror("Read failed");
                break;
            }
        }                  
        }
    }
    std::string str;
    for (int i = 0; i < buff.size(); i++) {
        str += buff[i];
        cout<<buff[i];
    }
    close(fd);
    return str;
    
}


//Script generation
void build_commands(string name,string Frequency_Min,string Frequency_Max,string cmp,string adp){
     PyRun_SimpleString("import sys");
    PyRun_SimpleString("sys.path.append('./')");

    // Load Python modules and functions
    PyObject* module = PyImport_ImportModule("commands");
    if (!module) {
        PyErr_Print();
    }

    PyObject* function = PyObject_GetAttrString(module, "commands");
    if (!function) {
        PyErr_Print();
        Py_DECREF(module);
    }

    PyObject* args = PyTuple_New(5);
    PyTuple_SetItem(args, 0, Py_BuildValue("s", name.c_str())); 
    PyTuple_SetItem(args, 1, Py_BuildValue("s", Frequency_Min.c_str())); 
    PyTuple_SetItem(args, 2, Py_BuildValue("s", Frequency_Max.c_str())); 
    PyTuple_SetItem(args, 3, Py_BuildValue("s", cmp.c_str()));
    PyTuple_SetItem(args, 4, Py_BuildValue("s", adp.c_str()));

    // Call the function
    PyObject* result = PyObject_CallObject(function, args);
    if (!result) {
        PyErr_Print();
        Py_DECREF(module);
        Py_DECREF(function);
        Py_DECREF(args);
    }
    // Free up resources
    Py_DECREF(result);
    Py_DECREF(module);
    Py_DECREF(function);
    Py_DECREF(args);
}



void threadFunc(vector<int> n,string direction) {
    //std::lock_guard<std::mutex> lock(mtx);
    int server_fd1, client_fd1;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[1024] = { 0 };

    // Create server socket
    if ((server_fd1 = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Set socket options
    if (setsockopt(server_fd1, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt failed");
        exit(EXIT_FAILURE);
    }

    // Bind socket to port
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);
    if (bind(server_fd1, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    // Listen for incoming connections
    if (listen(server_fd1, 3) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }
    // while(true){
    while(1){
    //std::lock_guard<std::mutex> lock(mtx);
    client_fd1 = accept(server_fd1, (struct sockaddr*)&address, (socklen_t*)&addrlen);
    if (client_fd1< 0) {
        perror("accept failed");
        exit(EXIT_FAILURE);
    }
    else if(client_fd1>0){break;}  
    }
    while(true){
    std::lock_guard<std::mutex> lock(mtx);
    char recv_buf0[64];
    vector<char> rec;
    while(1){        
        int recv_len =recv(client_fd1, recv_buf0, 64, 0);
        if(recv_len<64){
            rec.insert(rec.end(),recv_buf0,recv_buf0+recv_len);
            break;
        }
        else{
                rec.insert(rec.end(),begin(recv_buf0),end(recv_buf0));
            }
        }
    string str = "";
    for (char c : rec) {
        str.insert(str.end(), c);
    }
    char value[n.size()];
    if(str=="read gpio"){
    for(int i=0;i<n.size();i++){
    gpio_export(n[i]);
    gpio_set_direction(n[i],direction);
    value[i]=gpio_read_valu(n[i]);
    }
    send(client_fd1, value, sizeof(value), 0); 
   } 
   //close(client_fd1);
    mtx.unlock();    
   }  
}

#define maxsize 64
int main(int argc, char* argv[])
{       
    int server_fd, client_fd;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[1024] = { 0 };
    Py_Initialize();
    // Create server socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Set socket options
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt failed");
        exit(EXIT_FAILURE);
    }

    // Bind socket to port
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8888);
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    // Listen for incoming connections
    if (listen(server_fd, 3) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }
    

    thread t(threadFunc,gpio_nums,"in"); 
while(true){ 
    // Accept incoming connection
    while(1){
    std::lock_guard<std::mutex> lock(mtx); 
    client_fd = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
    if (client_fd< 0) {
        perror("accept failed");
        exit(EXIT_FAILURE);
    }
    else if(client_fd>0){break;}   
    }
    char recv_buf0[maxsize];
    vector<char> rec;
    int recv_len;
    while(1){
         // std::lock_guard<std::mutex> lock(mtx);          
        recv_len =recv(client_fd, recv_buf0, maxsize, 0);     
        if(recv_len<maxsize){
            rec.insert(rec.end(),recv_buf0,recv_buf0+recv_len);
            break;
        }
        else{
                rec.insert(rec.end(),begin(recv_buf0),end(recv_buf0));
            }
        }
    if(recv_len==0){
    close(client_fd);
    mtx.unlock();
    }
    else
    {
    string total = "";
    for (int i = 0; i < 3 && i < rec.size(); i++) {
        total += rec[i];
    }
    rec.erase(rec.begin(), rec.begin() + 3);
    int size;
    vector<vector<char> > result;
    int num=0;
    uint8_t tx2[4096] = {0};
    uint8_t rx2[4096];
    char value2;
    gpio_export(gpio_num);
    gpio_set_direction(gpio_num,"in"); 
    vector<uint8_t> list;
    if(total=="spi")
    {
    int fd;
	int status;
	int m1;
	int m2;
	int m3;
    const char *device = "/dev/spidev1.0";
    // Open the device file
    fd = open(device, O_RDWR);
    if (fd < 0) {
        perror("Failed to open SPI device");
        exit(1);
    }
    // Configure the SPI interface
    uint8_t mode = SPI_MODE_3;
    uint32_t speed = 24000000; // 24MHz
    uint8_t bits_per_word=8; 
	m1=ioctl(fd, SPI_IOC_WR_MODE, &mode);
	m2=ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
	m3=ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits_per_word);
    if ( m1< 0 ) {
        perror("Failed to set SPI mode");
        exit(1);
    }
    if ( m2< 0) {
        perror("Failed to set SPI speed");
        exit(1);
    }
	if ( m3< 0) {
        perror("Failed to set SPI speed");
        exit(1);
    }       
    if(rec[0]=='\x03'){
        string str1(rec.begin() + 1, rec.end()); 
        string filename=str1.substr(str1.length() - 4);   
        if(filename==".bin" || filename==".cal"){
    
        ifstream in = ifstream("/home/debian/bin/"+str1, ios::binary);
	if (!in.is_open()){cout << "Error: File Path is Wrong" << endl;}
    long long Beg = in.tellg();
	in.seekg(0, ios::end);
	long long End = in.tellg();
	long long fileSize = End - Beg;
	in.seekg(0, ios::beg);
    cout<<fileSize<<endl;
    int n=fileSize/2048+1;
	int byteBeenRead = 0;
	unsigned char temp;
    vector<char> result1;
    int qt=0;
    int h=0;

    // Read files (1 byte per cycle)
	while (in.read((char*)&temp, 1))
	{
		// Read 2048 bytes
		if (byteBeenRead % 2048 == 0)
		{
			//
            result1.push_back(58);
            result1.push_back(67);
            result1.push_back(72);
            result1.push_back(75);
            result1.push_back(61);
			
            qt ^=':';
            qt ^='C';
            qt ^='H';
            qt ^='K';
            qt ^='=';
            int qt1;
            if(qt==0){qt1=255;}
            else{qt1=qt-1;}

            stringstream ss;
            ss << setw(5) << setfill('0') << (qt1); 
            string str = ss.str();
            for (int i=0;i<str.size();i++)
               {
                result1.push_back(str[i]);
                }
            result1.push_back(4);
            qt=0;
            result.push_back(result1);
            vector<char>().swap(result1);	
            result1.push_back(3);
            h +=1;
            if(filename==".bin"){ 

            unsigned char byte1=((h) >> 8)& 0xFF;
            unsigned char byte2=(h) & 0xFF;
            qt ^= byte1;
            qt ^= byte2;
            result1.push_back(byte1);
            result1.push_back(byte2);
            }
            else{
                 qt ^=h;
            result1.push_back(h);
             if(h==n){
            int n_end=fileSize-(n-1)*2048;
            unsigned char byte1=(n_end)& 0xFF;
            unsigned char byte2=((n_end) >> 8) & 0xFF;
            unsigned char byte3=((n_end) >> 16)& 0xFF;
            unsigned char byte4=((n_end) >> 24)& 0xFF;
            result1.push_back(byte1);
            result1.push_back(byte2);
            result1.push_back(byte3);
            result1.push_back(byte4);
            qt ^= byte2;
            qt ^= byte1;
            qt ^= byte3;
            qt ^= byte4;
     }
            }
		    }
		byteBeenRead++;
		
		// Read 1 byte
		int hex = (unsigned)temp;
        qt ^=hex;
        result1.push_back(hex);
        //Less than 2048 parts
        if(byteBeenRead==fileSize){
            result1.push_back(58);
            result1.push_back(67);
            result1.push_back(72);
            result1.push_back(75);
            result1.push_back(61);
			
            qt ^=':';
            qt ^='C';
            qt ^='H';
            qt ^='K';
            qt ^='=';
            int qt2;
            if(qt==0){qt2=255;}
            else{qt2=qt-1;}
            stringstream ss;
            ss << setw(5) << setfill('0') << (qt2); 
            string str = ss.str();
            for (int i=0;i<str.size();i++)
            {
            result1.push_back(str[i]);
            }
            result1.push_back(4);
            result.push_back(result1);
        }
	}
	in.close();
num = result.size();
for(int k = 1;k < num;k++)
{
    uint8_t data[result[k].size()];
    int i = 0;
    for (int j=0;j<result[k].size();j++)
    {
        data[i++]=(int)result[k][j];
    }
    uint8_t rx[sizeof(data)];
    char value;
while(true){
    // Read the status of the GPIO pin
    value = gpio_read_valu(gpio_num);
    if(value=='1'){break;}
    }
    struct spi_ioc_transfer tr1={
    .tx_buf = (unsigned long)data,
    .rx_buf = (unsigned long)rx,
    .len = sizeof(data),
    .speed_hz = speed,
    .delay_usecs = 0,
    .bits_per_word = 8,
    .cs_change = 0,
    };
    // Send data
	status = ioctl(fd, SPI_IOC_MESSAGE(1), &tr1);
    if (status<0) {
        perror("Failed to send SPI message2");
        exit(1);
    }
while(true)
{
    while(true)
{
    // Read the status of the GPIO pin
    value2=gpio_read_valu(gpio_num);
    if(value2=='1'){break;}
 } 
    struct spi_ioc_transfer tr3={
    .tx_buf = (unsigned long)tx2,
    .rx_buf = (unsigned long)rx2,
    .len = sizeof(tx2),
    .speed_hz = speed,
    .delay_usecs = 0,
    .bits_per_word = 8,
    .cs_change = 0,
    };
    // send data
	status=ioctl(fd, SPI_IOC_MESSAGE(1), &tr3);
    if (status<0) {
        perror("Failed to send SPI message2");
        exit(1);
    }
    send(client_fd, rx2, sizeof(rx2), 0); 
    auto iter = std::find(std::begin(rx2), std::end(rx2), '\x04');
    if (iter != std::end(rx2)){break;}
 }
 } 
    uint8_t* pRx2 = rx2; 
    list.insert(list.end(), pRx2, pRx2 + sizeof(rx2) / sizeof(uint8_t));
    auto it = find(list.begin(), list.end(), '\x04'); 
    list.erase(it+1, list.end());
    char send_buf[list.size()];
    std::copy(list.begin(),list.end(),send_buf);
    int send_len = 0;
    send_len = send(client_fd, send_buf, sizeof(send_buf), 0);
	if (send_len < 0) {
		printf("Failed to receive");	
	}
} 
else if(filename=="HRST" || filename=="MRST") {
    std::chrono::milliseconds ms(100);
    if (filename=="HRST"){
    gpio_export(HRST);
    gpio_set_direction(HRST,"out");
    gpio_write_value(HRST,'0');
    std::this_thread::sleep_for(ms);
    gpio_write_value(HRST,'1');
    }
    else{
    gpio_export(MRST);
    gpio_set_direction(MRST,"out");
    gpio_write_value(MRST,'0');
    std::this_thread::sleep_for(ms);
    gpio_write_value(MRST,'1');
    }
}  
else if(filename==".txt"){
    commands_test(str1);
    char repy1[]={'O','K'};
    send(client_fd, repy1, sizeof(repy1), 0);
} 
else{
    istringstream ss(str1);
    vector<string> parts;
    string part;
    while (ss >> part) {
        parts.push_back(part);
    }
    build_commands(parts[0],parts[1],parts[2],parts[3],parts[4]);
    char repy[]={'O','K'};
    int cdf=send(client_fd, repy, sizeof(repy), 0);
    }
    }
    else{
        size=4096;
        result = splitArray(rec, size);
        num = result.size();
         for(int k = 0;k < num;k++)
{
    uint8_t data[result[k].size()];
    int i = 0;
    for (int j=0;j<result[k].size();j++)
    {
        data[i++]=(int)result[k][j];
    }
    uint8_t rx[sizeof(data)];
    char value;
while(true){
    // Read the status of the GPIO pin
    value = gpio_read_valu(gpio_num);
    if(value=='1'){break;}
    }
    struct spi_ioc_transfer tr1={
    .tx_buf = (unsigned long)data,
    .rx_buf = (unsigned long)rx,
    .len = sizeof(data),
    .speed_hz = speed,
    .delay_usecs = 0,
    .bits_per_word = 8,
    .cs_change = 0,
    };
    // Send data
	status = ioctl(fd, SPI_IOC_MESSAGE(1), &tr1);
    if (status<0) {
        perror("Failed to send SPI message2");
        exit(1);
    }
} 
while(true)
{
    while(true)
{
    // Read the status of the GPIO pin
    value2=gpio_read_valu(gpio_num);
    if(value2=='1'){break;}
 } 
    struct spi_ioc_transfer tr3={
    .tx_buf = (unsigned long)tx2,
    .rx_buf = (unsigned long)rx2,
    .len = sizeof(tx2),
    .speed_hz = speed,
    .delay_usecs = 0,
    .bits_per_word = 8,
    .cs_change = 0,
    };
    // send data
	status=ioctl(fd, SPI_IOC_MESSAGE(1), &tr3);
    if (status<0) {
        perror("Failed to send SPI message2");
        exit(1);
    }
    uint8_t* pRx2 = rx2; 
    list.insert(list.end(), pRx2, pRx2 + sizeof(rx2) / sizeof(uint8_t));
    auto iter = std::find(std::begin(rx2), std::end(rx2), '\x04');
    if (iter != std::end(rx2)){break;}
 }
    auto it = find(list.begin(), list.end(), '\x04'); 
    list.erase(it+1, list.end());
    char send_buf[list.size()];
std::copy(list.begin(),list.end(),send_buf);
int send_len = 0;
send_len = send(client_fd, send_buf, sizeof(send_buf), 0);
		if (send_len < 0) {
			printf("Failed to receive");	
		}
    }  
    cout<<rx2<<endl;
    close(fd);
    close(client_fd);
    mtx.unlock();
    //close(server_fd); 
 }

   else{
    string command = "";
    for (int i = 0; i <rec.size(); i++) {
        command += rec[i];
    }
    string str = UART_connection(command);
    int len = str.length();
    unsigned char charArray[len];
    for (int i = 0; i < len; i++) {
        charArray[i] = static_cast<unsigned char>(str[i]);
    }
    send(client_fd, charArray, sizeof(charArray), 0); 
    close(client_fd);
    mtx.unlock();
   }
    }
      }
    return 0;
}

