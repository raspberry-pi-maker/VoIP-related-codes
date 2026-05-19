/*
"If this code works, it was written by spypiggy. If not, I don't know who wrote it."
In honor of Paul DiLascia.
*/
#include"global.h"
#include"epoll.h"
#include"sip_util.h"

int g_siprec_port = 5070;
int g_siprec_sock;
std::queue<string> g_siprec_queue;
std::mutex g_sip_mutex;   // pthread_mutex_t → std::mutex
sockaddr_in g_siprec_addr;
socklen_t g_siprec_addr_len;
std::condition_variable g_cv_siprec;

int main(int argc, char * argv[])
{
    std::cout << "====== Simple siprec server(for mod_siprec V1.34) Start ======" << std::endl;

    init_epoll();

    g_siprec_sock = buildsocket(g_siprec_port);
    if(!g_siprec_sock){
        std::cout << "epoll siprec socket port[" << g_siprec_sock  << "] build failed => exit process" << std::endl;
        return 0;
    }
    std::cout << "Initialize epoll success epoll siprec sock[" << g_siprec_sock << "]" << std::endl;

    // pthread_create + pthread_detach → std::thread
    std::thread p_thread([](){
        epoll_thread(nullptr);   // epoll_thread function
    });
    p_thread.detach();

    std::cout << "Create UDP sockets success" << std::endl;
    int size;
    std::string data;
    while(1){
        //bool has_data = false;
        std::string data;
        std::unique_lock<std::mutex> lock(g_sip_mutex);
        g_cv_siprec.wait(lock, []{ return !g_siprec_queue.empty(); });
        data = std::move(g_siprec_queue.front()); // Improve performance by moving instead of copying
        g_siprec_queue.pop();
        lock.unlock();
        do_siprec_message(data.c_str()); 
    }        
    return 0;
}
