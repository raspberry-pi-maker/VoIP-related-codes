#include "global.h"
#include "util.h"
#include <libgen.h>   // header containing the dirname()
#include <sys/file.h>   // header containing the flock function declaration

static short seg_aend[8] = {0x1F, 0x3F, 0x7F, 0xFF,
			    0x1FF, 0x3FF, 0x7FF, 0xFFF};
static short seg_uend[8] = {0x3F, 0x7F, 0xFF, 0x1FF,
			    0x3FF, 0x7FF, 0xFFF, 0x1FFF};
static short search(short val, short *table, short size)
{
   short i;
   
   for (i = 0; i < size; i++) {
      if (val <= *table++)
	 return (i);
   }
   return (size);
}

void _sleep_ns(int secs, long nsecs) {
	if (nsecs > 999999999) {
		secs += nsecs/1000000000;
		nsecs = nsecs % 1000000000;
	}
	{
		struct timespec ts = { secs, nsecs };
		nanosleep(&ts, NULL);
	}
}

void sleep_ns(long nsecs) { _sleep_ns(0, nsecs); }
void sleep_ms(int msecs) { sleep_ns(msecs*1000000); }
void sleep_s(int secs) { _sleep_ns(secs, 0); }

/*
in the sdp find the line that contains the token (copies from the token)
*/
int getline(const char *pSDP, const char *szTok, char *pLine)
{
	const char *pStart = strstr(pSDP, szTok);
	if(NULL == pStart) return -1;
	while(*pStart != '\r' && *pStart != '\n'){
		*pLine = *pStart;
		pLine++;
		pStart++;
	}
	*pLine = 0x00;
	return 0;
}


std::string ltrim(const std::string &s) {
    return std::regex_replace(s, std::regex("^\\s+"), std::string(""));
}

std::string rtrim(const std::string &s) {
    return std::regex_replace(s, std::regex("\\s+$"), std::string(""));
}

std::string trim(const std::string &s) {
    return ltrim(rtrim(s));
}

int trim(const char *in, char *out)
{
    string s(in);
    s = trim(s);
    strcpy(out, s.c_str());
    return 0;
}


const char *ip_to_str(const u_char *ip, char *pstr)
{
  sprintf(pstr, "%d.%d.%d.%d", (int)ip[0], (int)ip[1], (int)ip[2], (int)ip[3]);
  return(const char *)pstr;
}

// this function converts pcmu (8bit) to pcm(16bit)
int16_t ulaw2linear(u_char u_val)
{
  int16_t t;
   /* Complement to obtain normal u-law value. */
   u_val = ~u_val;
   
   /*
    * Extract and bias the quantization bits. Then
    * shift up by the segment number and subtract out the bias.
    */
   t = ((u_val & QUANT_MASK) << 3) + BIAS;
   t <<= ((unsigned)u_val & SEG_MASK) >> SEG_SHIFT;
   
   return ((u_val & SIGN_BIT) ? (BIAS - t) : (t - BIAS));  
}

// this function converts pcma (8bit) to pcm(16bit)
int16_t alaw2linear(u_char	a_val)
{
   int16_t t;
   int16_t seg;
   
   a_val ^= 0x55;
   
   t = (a_val & QUANT_MASK) << 4;
   seg = ((unsigned)a_val & SEG_MASK) >> SEG_SHIFT;
   switch (seg) {
   case 0:
      t += 8;
      break;
   case 1:
      t += 0x108;
      break;
   default:
      t += 0x108;
      t <<= seg - 1;
   }
   return ((a_val & SIGN_BIT) ? t : -t);
}

unsigned char linear2alaw(short pcm_val)	/* 2's complement (16-bit range) */
{
   short	 mask;
   short	 seg;
   unsigned char aval;
   
   pcm_val = pcm_val >> 3;

   if (pcm_val >= 0) {
      mask = 0xD5;		/* sign (7th) bit = 1 */
   } else {
      mask = 0x55;		/* sign bit = 0 */
      pcm_val = -pcm_val - 1;
   }
   
   /* Convert the scaled magnitude to segment number. */
   seg = search(pcm_val, seg_aend, 8);
   
   /* Combine the sign, segment, and quantization bits. */
   
   if (seg >= 8)		/* out of range, return maximum value. */
      return (unsigned char) (0x7F ^ mask);
   else {
      aval = (unsigned char) seg << SEG_SHIFT;
      if (seg < 2)
	 aval |= (pcm_val >> 1) & QUANT_MASK;
      else
	 aval |= (pcm_val >> seg) & QUANT_MASK;
      return (aval ^ mask);
   }
}

unsigned char linear2ulaw(short pcm_val)	/* 2's complement (16-bit range) */
{
   short         mask;
   short	 seg;
   unsigned char uval;
   
   /* Get the sign and the magnitude of the value. */
   pcm_val = pcm_val >> 2;
   if (pcm_val < 0) {
      pcm_val = -pcm_val;
      mask = 0x7F;
   } else {
      mask = 0xFF;
   }
   if ( pcm_val > CLIP ) pcm_val = CLIP;		/* clip the magnitude */
   pcm_val += (BIAS >> 2);
   
   /* Convert the scaled magnitude to segment number. */
   seg = search(pcm_val, seg_uend, 8);
   
   /*
   * Combine the sign, segment, quantization bits;
   * and complement the code word.
   */
   if (seg >= 8)		/* out of range, return maximum value. */
      return (unsigned char) (0x7F ^ mask);
   else {
      uval = (unsigned char) (seg << 4) | ((pcm_val >> (seg + 1)) & 0xF);
      return (uval ^ mask);
   }
}


int16_t pcm_mix(int a, int b)
{
    int m;
    // Make both samples unsigned (0..65535)
    a += 32768;
    b += 32768;

    // Pick the equation
    if ((a < 32768) || (b < 32768)) {
        // Viktor's first equation when both sources are "quiet"
        // (i.e. less than middle of the dynamic range)
        m = a * b / 32768;
    } else {
        // Viktor's second equation when one or both sources are loud
        m = 2 * (a + b) - (a * b) / 32768 - 65536;
    }

    // Output is unsigned (0..65536) so convert back to signed (-32768..32767)
    if (m == 65536) m = 65535;
    m -= 32768;
    return (int16_t)m;
}

/*
Use the popen function instead of the system function to receive stdout output.
*/
string exec_command(const char *cmd)
{
    string retstr("");
    char buffer[1024];

    FILE* pipe = popen(cmd, "r");
    if (pipe){
        try {
            while (!feof(pipe)) {
                if (fgets(buffer, sizeof(buffer), pipe) != NULL)
                    retstr += buffer;
            }
        } catch (...) {
        }
        pclose(pipe);
    }
    return retstr;
}

/*
Resources (file descriptors) are automatically released when the program terminates.
*/
bool lock_pidfile(const char *name)
{
    int pid_fd = open(name, O_RDWR | O_CREAT, 0644);
    if (pid_fd < 0) {
        std::cerr << "Cannot open PID file" << std::endl;
        return false;
    }

    // 파일 잠금 시도
    if (flock(pid_fd, LOCK_EX | LOCK_NB) < 0) {
        std::cerr << "It is already running!" << std::endl;
        return false;
    }

    ftruncate(pid_fd, 0);
    std::string pid_str = std::to_string(getpid());
    write(pid_fd, pid_str.c_str(), pid_str.size());
    return true;
}

void get_exe_path(char *buffer, int buf_size)
{
    char path[PATH_MAX];
    buffer[0] = 0x00;
    if(PATH_MAX > buf_size) return;

    ssize_t count = readlink("/proc/self/exe", path, PATH_MAX);
    if (count == -1) {
        perror("readlink");
    }
    path[count] = '\0';  // Add null character to the end of the string

    // Since dirname() directly modifies the string, it is safe to use a copy.
    char dirpath[PATH_MAX];
    snprintf(dirpath, sizeof(dirpath), "%s", path);
    strcpy(buffer,  dirname(dirpath));
    std::cout << "executable file location: " << buffer << std::endl;
}

bool file_exist(const char *filename)
{
	struct stat   buffer;
	return (stat (filename, &buffer) == 0);
}

