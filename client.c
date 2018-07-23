/*

	Open Source Amoveo OpenCL Miner
	
	for AMD & Nvidia GPU´s

	Donations:
	BONJmlU2FUqYgUY60LTIumsYrW/c6MHte64y5KlDzXk5toyEMaBzWm8dHmdMfJmXnqvbYmlwim0hiFmYCCn3Rm0=

*/

#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <CL/cl.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/time.h>

#include "sha256.h"

#define MAXGPU          32
#define NONCESIZE  	23

cl_device_id device_id[32];

int randfd;
pthread_mutex_t mutex;

int diff;
int sharediff = 0;

unsigned char data[32];

unsigned int poolport;
unsigned char *workPath;
unsigned char *address;
unsigned char *hostname;
struct in_addr *poolip;
int pollcount = 0;

static char encoding_table[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'};
static char *decoding_table = NULL;
static int mod_table[] = {0, 2, 1};

void build_decoding_table() {
    decoding_table = malloc(256);

    for (int i = 0; i < 64; i++)
        decoding_table[(unsigned char) encoding_table[i]] = i;
}

char *base64_encode(const unsigned char *data, size_t input_length, size_t *output_length) {

    *output_length = 4 * ((input_length + 2) / 3);

    char *encoded_data = malloc(*output_length);
    if (encoded_data == NULL) return NULL;

    for (int i = 0, j = 0; i < input_length;) {

        uint32_t octet_a = i < input_length ? (unsigned char)data[i++] : 0;
        uint32_t octet_b = i < input_length ? (unsigned char)data[i++] : 0;
        uint32_t octet_c = i < input_length ? (unsigned char)data[i++] : 0;

        uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;

        encoded_data[j++] = encoding_table[(triple >> 3 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 2 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 1 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 0 * 6) & 0x3F];
    }

    for (int i = 0; i < mod_table[input_length % 3]; i++)
        encoded_data[*output_length - 1 - i] = '=';

    return encoded_data;
}


unsigned char *base64_decode(const char *data, size_t input_length, size_t *output_length) {
    if (decoding_table == NULL) build_decoding_table();

    if (input_length % 4 != 0) return NULL;

    *output_length = input_length / 4 * 3;
    if (data[input_length - 1] == '=') (*output_length)--;
    if (data[input_length - 2] == '=') (*output_length)--;

    unsigned char *decoded_data = malloc(*output_length);
    if (decoded_data == NULL) return NULL;

    for (int i = 0, j = 0; i < input_length;) {

        uint32_t sextet_a = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];
        uint32_t sextet_b = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];
        uint32_t sextet_c = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];
        uint32_t sextet_d = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];

        uint32_t triple = (sextet_a << 3 * 6)
        + (sextet_b << 2 * 6)
        + (sextet_c << 1 * 6)
        + (sextet_d << 0 * 6);

        if (j < *output_length) decoded_data[j++] = (triple >> 2 * 8) & 0xFF;
        if (j < *output_length) decoded_data[j++] = (triple >> 1 * 8) & 0xFF;
        if (j < *output_length) decoded_data[j++] = (triple >> 0 * 8) & 0xFF;
    }

    return decoded_data;
}

void base64_cleanup() {
    free(decoding_table);
}

void nonblock(int sock) {
	int flags = fcntl(sock, F_GETFL, 0);
	flags = flags | O_NONBLOCK;
	fcntl(sock, F_SETFL, flags);
}

int pool() {
        int sd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

	nonblock(sd);

	struct sockaddr_in servaddr;

        memset(&servaddr, 0, sizeof(servaddr));

        servaddr.sin_family = AF_INET;
        servaddr.sin_port = htons(poolport);
	memcpy(&servaddr.sin_addr, poolip, sizeof(poolip));

	pollcount++;

        connect(sd, (struct sockaddr *)&servaddr, sizeof(servaddr));

	char buffer[1000];

	/*	if(strlen(workPath) > 3) {	
		sprintf(buffer, "POST %s HTTP/1.1\r\ncontent-type: application/octet-stream\r\ncontent-length: %i\r\nhost: %s:%i\r\nconnection: close\r\n\r\n[\"mining_data\", \"%s\"]", 
			workPath,
			(19 + (int)strlen(address)),
			hostname,
			poolport,
			address
		);
	} else {
		sprintf(buffer, "POST %s HTTP/1.1\r\ncontent-type: application/octet-stream\r\ncontent-length: %i\r\nhost: %s:%i\r\nconnection: close\r\n\r\n[\"mining_data\",8080]", 
			workPath,
			20,
			hostname,
			poolport
		);
		} */
	sprintf(buffer, "POST %s HTTP/1.1\r\ncontent-type: application/octet-stream\r\ncontent-length: %i\r\nhost: %s:%i\r\nconnection: close\r\n\r\n[\"mining_data\", \"%s\"]", 
		workPath,
		(19 + (int)strlen(address)),
		hostname,
		poolport,
		address
	);
	

	struct timeval tv;
	tv.tv_sec = 4;
	tv.tv_usec = 0;

	fd_set my;
	FD_ZERO(&my);
	FD_SET(sd, &my);
	select(sd+1, NULL, &my, NULL, &tv);

	if(!FD_ISSET(sd, &my)) {
		close(sd);
		usleep(500000);
		return -1;
	}

	send(sd, buffer, strlen(buffer), MSG_NOSIGNAL);

	tv.tv_sec = 4;
	tv.tv_usec = 0;

	FD_ZERO(&my);
	FD_SET(sd, &my);

	select(sd+1, &my, NULL, NULL, &tv);

	if(!FD_ISSET(sd, &my)) {
		close(sd);
		usleep(500000);
		return -1;
	}

	int bytes = read(sd, buffer, 1000);
        close(sd);

	for(int i=0; i<bytes-8; i++) {
		if(buffer[i] == '\r' && buffer[i+1] == '\n' && buffer[i+2] == '\r' && buffer[i+3] == '\n') {
			int sp = 0;

			while(buffer[i+4] != '[') i++;

			for(int j=i+11; j<bytes; j++) {
				if(buffer[j] == '"') {
					if(sp == 0) {
						sp = j;
					} else {
						int olen;
						char *bf = base64_decode(&buffer[sp + 1], j - sp - 1, &olen);
							
						buffer[j+7] = 0;

						if(buffer[j+12] == ']')
							buffer[j+12] = 0;
						else
							buffer[j+13] = 0;

						diff = strtol(&buffer[j+2], (char **)NULL, 10);
						sharediff = strtol(&buffer[j+8], (char **)NULL, 10);

						if (strncmp(data, bf, 32)!=0) {						
							printf("Network diff: %i, share diff: %i\n", diff, sharediff);
							memcpy(data, bf, 32);
						}

						return 0;
					}
				}
			}
			i = bytes;
		}
	}
	usleep(500000);

	return -1;
}

int submitnonce(char *nonce) {
        int sd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

 	nonblock(sd);

	struct sockaddr_in servaddr;

        memset(&servaddr, 0, sizeof(servaddr));

        servaddr.sin_family = AF_INET;
        servaddr.sin_port = htons(poolport);
	memcpy(&servaddr.sin_addr, poolip, sizeof(poolip));
        connect(sd, (struct sockaddr *)&servaddr, sizeof(servaddr));

	char buffer[1000];

	int olen;
	char *nonce64 = base64_encode(nonce, NONCESIZE, &olen);

	nonce64[olen] = 0;

	sprintf(buffer, "POST %s HTTP/1.1\r\ncontent-type: application/octet-stream\r\ncontent-length: %i\r\nhost: %s:%i\r\nconnection: close\r\n\r\n[\"work\",\"%s\",\"%s\"]", workPath, 14 + (int)strlen(address) + olen, hostname, poolport, nonce64, address);

	struct timeval tv;
	tv.tv_sec = 6;
	tv.tv_usec = 0;

	fd_set my;
	FD_ZERO(&my);
	FD_SET(sd, &my);
	select(sd+1, NULL, &my, NULL, &tv);

	if(!FD_ISSET(sd, &my)) {
		//printf("Submit Connect timeout. Retrying..\n");
		close(sd);
		return -1;
	}

	send(sd, buffer, strlen(buffer), MSG_NOSIGNAL);

	tv.tv_sec = 6;
	tv.tv_usec = 0;

	FD_ZERO(&my);
	FD_SET(sd, &my);

	select(sd+1, &my, NULL, NULL, &tv);

	if(!FD_ISSET(sd, &my)) {
		//printf("Submit Recv timeout. Retrying..\n");
		close(sd);
		return -1;
	}

	int bytes = read(sd, buffer, 1000);
	close(sd);

	for(int i=0; i<bytes - 5; i++)
		if(buffer[i] == '\r' && buffer[i+1] == '\n' && buffer[i+2] == '\r' && buffer[i+3] == '\n') {
			if(buffer[i+4] == '[' && buffer[bytes-1] == ']') {
				printf("Share accepted.\n");
			}
			//write(1, &buffer[i+4], bytes - i - 4);
		}
	return 0;
}

/*

  Functions copied from Zack´s C-Miner:
  https://github.com/zack-bitcoin/amoveo-c-miner

*/

static WORD pair2sci(WORD l[2]) {
	return((256*l[0]) + l[1]);
}

WORD hash2integer(BYTE h[32]) {
  WORD x = 0;
  WORD z = 0;
  for (int i = 0; i < 31; i++) {
    if (h[i] == 0) {
      x += 8;
      continue;
    } else if (h[i] < 2) {
      x += 7;
      z = h[i+1];
    } else if (h[i] < 4) {
      x += 6;
      z = (h[i+1] / 2) + ((h[i] % 2) * 128);
    } else if (h[i] < 8) {
      x += 5;
      z = (h[i+1] / 4) + ((h[i] % 4) * 64);
    } else if (h[i] < 16) {
      x += 4;
      z = (h[i+1] / 8) + ((h[i] % 8) * 32);
    } else if (h[i] < 32) {
      x += 3;
      z = (h[i+1] / 16) + ((h[i] % 16) * 16);
    } else if (h[i] < 64) {
      x += 2;
      z = (h[i+1] / 32) + ((h[i] % 32) * 8);
    } else if (h[i] < 128) {
      x += 1;
      z = (h[i+1] / 64) + ((h[i] % 64) * 4);
    } else {
      z = (h[i+1] / 128) + ((h[i] % 128) * 2);
    }
    break;
  }
  WORD y[2];
  y[0] = x;
  y[1] = z;
  return(pair2sci(y));
}

/* Copy end */

struct gpuContext {
        size_t local;                    
        cl_context context;              
        cl_command_queue commands;       
        cl_program program;              
        cl_kernel kernel;                
        cl_mem input;                    
        cl_mem output;                   
        pthread_mutex_t gpulock;
        pthread_t thread;
        int gpuid;
        int rrun;
	int blocksdone;
	int waitus;
	int load;
	char *name;
};

void *worker(void *p1) {
        struct gpuContext *gc = (struct gpuContext*)p1;
        int err;
	BYTE nonces[NONCESIZE];

	size_t global = gc->load;

        char *in  = (char*)malloc(55);
        char *out = (char*)malloc(NONCESIZE * gc->load);

        for(;;) {
                pthread_mutex_lock(&gc->gpulock);

		read(randfd, nonces, NONCESIZE);

		for (int i = 0; i < 32; i++)
			in[i] = data[i];

		nonces[4] = 0; nonces[5] = 0; nonces[6] = 0; nonces[7] = 0;
		nonces[12] = 0; nonces[13] = 0; nonces[14] = 0; nonces[15] = 0;

		for(int i = 0; i < NONCESIZE; i++)
			in[i+32] = nonces[i];

                err = clEnqueueWriteBuffer(gc->commands, gc->input, CL_TRUE, 0, 55, in, 0, NULL, NULL);
                if (err != CL_SUCCESS) {
                        exit(1);
                }

                err = clEnqueueNDRangeKernel(gc->commands, gc->kernel, 1, NULL, &global, NULL, 0, NULL, NULL);
                if (err) {
                        exit(1);
                }

                struct timeval kernelStart, kernelEnd;
                gettimeofday(&kernelStart,NULL);
#ifdef CLWAIT			
                usleep(gc->waitus * 0.95);
#endif

                clFinish(gc->commands);

                gettimeofday(&kernelEnd,NULL);
                unsigned long time_in_micros = 1000000 * (kernelEnd.tv_sec - kernelStart.tv_sec) + (kernelEnd.tv_usec - kernelStart.tv_usec);
                gc->waitus = time_in_micros;

                err = clEnqueueReadBuffer( gc->commands, gc->output, CL_TRUE, 0, 8 * gc->load, out, 0, NULL, NULL );
                if (err != CL_SUCCESS) {
                        printf("Error: Failed to read output array! %d\n", err);
                        exit(1);
                }
		gc->blocksdone++;

                pthread_mutex_unlock(&gc->gpulock);

		char hash0[32];
		SHA256_CTX ctx;

		for(int i=0; i<gc->load; i++) {
			if(
				out[i * 8 + 0] == 0 && out[i * 8 + 1] == 0 &&
				out[i * 8 + 2] == 0 && out[i * 8 + 3] == 0 &&
				out[i * 8 + 4] == 0 && out[i * 8 + 5] == 0 &&
				out[i * 8 + 6] == 0 && out[i * 8 + 7] == 0
			) continue;

			char submit[23];
			memcpy(submit, &in[32], 16);
			memcpy(&submit[16], &out[i * 8], 7);

			sha256_init(&ctx);
			sha256_update(&ctx, data, 32);
			sha256_update(&ctx, submit, 23);
			sha256_final(&ctx, hash0);

			int xxx = hash2integer(hash0);

			if(xxx >= sharediff) {
				printf("Share with difficulty %d found. Submitting ...\n", xxx);
				while(submitnonce(submit) == -1);
			}
		}
	}
}

int main(int argc, char **argv) {
	// Default-pool:
	poolport = 8880;
	hostname = "veopool.pw";
	workPath = "/work";

        if (argc > 1) {
		size_t lengthAddress = strlen(argv[1])+1;

		if(argc > 2) {
			for(int i=0;i<strlen(argv[2]);i++) {
				if(argv[2][i] == ':' || argv[2][i] == '/') {
					hostname = (char*)malloc(i+1);
					memcpy(hostname, argv[2], i);
					hostname[i] = 0;
					if(argv[2][i] == ':') {
						poolport = atoi(&argv[2][i+1]);
						for(;i<strlen(argv[2]) && argv[2][i] != '/';i++);
					}

					workPath = &argv[2][i];
					i = 100000;
				}
			}
		}

		address = (char *)malloc(strlen(argv[1]) + 1);
		strcpy(address, argv[1]);
	} else {
		printf("Usage: ./veoCL address poolip:port/path\n");
		address = "BONJmlU2FUqYgUY60LTIumsYrW/c6MHte64y5KlDzXk5toyEMaBzWm8dHmdMfJmXnqvbYmlwim0hiFmYCCn3Rm0=";
	}
	if (strlen(workPath) < 1) {
		workPath = "/";
	}


	printf("Mining to Address: %s\n",address);
	printf("Mining to Pool: %s:%i%s\n",hostname,poolport,workPath);

	// Resolve IP
	struct hostent *host = gethostbyname(hostname);

	if(host == NULL || host->h_addr_list == NULL) {
		printf("Could not connect to pool.\n");
		return -1;			
	}

	memcpy(&poolip, &host->h_addr_list[0], sizeof(poolip));
	
	int err;

	pthread_mutex_init(&mutex, NULL);

        randfd = open("/dev/urandom", O_RDONLY);

        unsigned char pad[32];
        SHA256_CTX ctx;

	unsigned char *ccd = malloc(20000);

	int fd = open("amoveo.cl", O_RDONLY);
	ccd[read(fd, ccd, 20000)] = 0;
	close(fd);

        cl_platform_id platform_id = NULL;
        cl_uint ret_num_platforms;
        cl_uint ret_num_devices;

        clGetPlatformIDs(1, &platform_id, &ret_num_platforms);
        err = clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_GPU, MAXGPU, &device_id, &ret_num_devices);
        if (err != CL_SUCCESS) {
                printf("Error: Failed to create a device group!\n");
                return EXIT_FAILURE;
        }

        struct gpuContext *c = (struct gpuContext*) malloc(sizeof(struct gpuContext) * ret_num_devices);

	printf("Getting work from Pool...\n");
	while(pool() == -1);
	printf("Got work. Starting miner...\n");

        for(int i=0; i<ret_num_devices; i++) {
		c[i].blocksdone = 0;
		c[i].waitus = 1;
                c[i].gpuid = i;
                c[i].rrun = 0;
		cl_uint maxComputeUnits;
		size_t workgroup;

		char gpuname[600];
		clGetDeviceInfo(device_id[i], CL_DEVICE_NAME, sizeof(gpuname), gpuname, NULL);
		clGetDeviceInfo(device_id[i], CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(maxComputeUnits), &maxComputeUnits, NULL);
		clGetDeviceInfo(device_id[i], CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(workgroup), &workgroup, NULL);
		printf("Device found: %s, using load %lu\n", gpuname, maxComputeUnits * workgroup);
		c[i].name = malloc(sizeof(gpuname)+1);
		memcpy(c[i].name, gpuname, sizeof(gpuname));
		c[i].name[sizeof(gpuname)] = 0;
		c[i].load = maxComputeUnits * workgroup * 2;

                c[i].context = clCreateContext(0, 1, &device_id[i], NULL, NULL, &err);
                if (!c[i].context) {
                        printf("No OpenCL devices found. Check your installation.!\n");
                        return EXIT_FAILURE;
                }

                c[i].commands = clCreateCommandQueue(c[i].context, device_id[i], 0, &err);
                if(!c[i].commands) {
                        printf("No OpenCL devices found. Check your installation.!\n");
                        return EXIT_FAILURE;
                }

                c[i].program = clCreateProgramWithSource(c[i].context, 1, (const char **) &ccd, NULL, &err);
                if (!c[i].program) {
			printf("Error compiling OpenCL kernel.!\n");
                        return EXIT_FAILURE;
                }

                err = clBuildProgram(c[i].program, 0, NULL, NULL, NULL, NULL);
                if (err != CL_SUCCESS) {
                        size_t len;
                        clGetProgramBuildInfo(c[i].program, device_id[i], CL_PROGRAM_BUILD_LOG, NULL, NULL, &len);
                        exit(1);
                }

                c[i].kernel = clCreateKernel(c[i].program, "amoveo", &err);
                if (!c[i].kernel || err != CL_SUCCESS) {
                        exit(1);
                }

                c[i].input = clCreateBuffer(c[i].context,  CL_MEM_READ_ONLY , 64, NULL, NULL);
                if (!c[i].input) {
                        exit(1);
                }

                c[i].output = clCreateBuffer(c[i].context,  CL_MEM_WRITE_ONLY, 8 * c[i].load, NULL, NULL);
                if (!c[i].output) {
                        exit(1);
                }

                err = 0;
                err  = clSetKernelArg(c[i].kernel, 0, sizeof(cl_mem), &c[i].input);
                err  = clSetKernelArg(c[i].kernel, 1, sizeof(cl_mem), &c[i].output);

                if (err != CL_SUCCESS) {
                        exit(1);
                }

                err = clGetKernelWorkGroupInfo(c[i].kernel, device_id[i], CL_KERNEL_WORK_GROUP_SIZE, sizeof(c[i].local), &c[i].local, NULL);

                if (err != CL_SUCCESS) {
                        exit(1);
                }

                pthread_mutex_init(&c[i].gpulock, NULL);
                pthread_create(&c[i].thread, NULL, worker, &c[i]);
        }

	struct timeval tv1, tv2;
	gettimeofday(&tv1, NULL);
	
	usleep(5000000);
	
        for(int run=0;;run++) {
		gettimeofday(&tv2, NULL);
		if(run % 3 == 0) {
			unsigned long long total = 0;
			for(int i=0; i < ret_num_devices; i++) {
				unsigned long long rate = (c[i].load * (1ULL << 15)) / c[i].waitus; total += rate;
				printf("GPU %u, %s: %lluMH/s ", i, c[i].name, rate);
			}
			printf("Total: %lluMH/s\n", total);
		}
		usleep(2000000);
		while(pool() == -1);
        }
}

