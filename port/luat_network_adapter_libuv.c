
#include "uv.h"
#include "luat_base.h"
#include "luat_log.h"
#include "luat_uart.h"
#include "luat_malloc.h"
#include "printf.h"
#include "luat_msgbus.h"

#include "luat_network_adapter.h"

#include <stdio.h>


#define LUAT_LOG_TAG "libuv"
#include "luat_log.h"

#define MAX_SOCK_NUM 8

enum
{
	EV_LIBUV_EVENT_START = USER_EVENT_ID_START + 0x2000000,
	EV_LIBUV_SOCKET_TX,
	EV_LIBUV_NETIF_INPUT,
	EV_LIBUV_TCP_TIMER,
	EV_LIBUV_COMMON_TIMER,
	EV_LIBUV_SOCKET_RX_DONE,
	EV_LIBUV_SOCKET_CREATE,
	EV_LIBUV_SOCKET_CONNECT,
	EV_LIBUV_SOCKET_DNS,
	EV_LIBUV_SOCKET_DNS_IPV6,
	EV_LIBUV_SOCKET_LISTEN,
	EV_LIBUV_SOCKET_ACCPET,
	EV_LIBUV_SOCKET_CLOSE,
	EV_LIBUV_NETIF_LINK_STATE,
	EV_LIBUV_DHCP_TIMER,
	EV_LIBUV_FAST_TIMER,
	EV_LIBUV_NETIF_SET_IP,
	EV_LIBUV_NETIF_IPV6_BY_MAC,
};

typedef struct
{
	uint64_t socket_tag;
	socket_ctrl_t socket[MAX_SOCK_NUM];
	CBFuncEx_t socket_cb;
	void *user_data;
	uint32_t socket_busy;
	uint32_t socket_connect;
	uint8_t dns_adapter_index;
	uint8_t network_ready;
	uint8_t next_socket_index;
} libuv_ctrl_c;

typedef struct uv_dns_query
{
    char domain[256];
    void* param;
    // libuv所需的数据结构
    struct addrinfo hints;
    uv_getaddrinfo_t resolver;
}uv_dns_query_t;

typedef struct uv_conn {
    uint64_t tag;
    uv_connect_t c;
    uv_tcp_t tcp;
    uv_udp_t udp;
    void* param;
    uint8_t is_ipv6;
    uint8_t is_tcp;
    char* recv_buff;
    size_t recv_size;
}uv_conn_t;

int libuv_init(uint8_t adapter_index);
int libuv_check_all_ack(int socket_id);
int libuv_set_link_state(uint8_t adapter_index, uint8_t updown);

#define CHECK_SOCKET_ID     if (socket_id < 0 || socket_id >= MAX_SOCK_NUM) {\
        LLOGE("socket id不合法 %d", socket_id);\
        return -1;\
    }\
    if (sockets[socket_id].tag != tag) {\
        LLOGE("socket tag 不匹配 %d %016X %016X", socket_id, sockets[socket_id].tag, tag);\
        return -1;\
    }

//---------------------------------------
//---------------------------------------

static libuv_ctrl_c ctrl;
extern uv_loop_t *main_loop;

static uv_conn_t sockets[MAX_SOCK_NUM];
static uint64_t socket_tag_counter = 0xFAFB;

static void cb_to_nw_task(uint32_t event_id, uint32_t param1, uint32_t param2, uint32_t param3)
{
	luat_network_cb_param_t param = {.tag = 0, .param = NULL};
	OS_EVENT event = { .ID = event_id, .Param1 = param1, .Param2 = param2, .Param3 = param3};
	if ((event_id > EV_NW_DNS_RESULT))
	{
		// if (event_id != EV_NW_SOCKET_CLOSE_OK)
		// {
			event.Param3 = sockets[param1].param;
			param.tag = sockets[param1].tag;
		// }
		// else
		// {
			// event.Param3 = ((luat_network_cb_param_t *)param3)->param;
			// param.tag = ((luat_network_cb_param_t *)param3)->tag;
		// }
	}
	// switch(event_id)
	// {
	// case EV_NW_SOCKET_CLOSE_OK:
	// case EV_NW_SOCKET_CONNECT_OK:
	// case EV_NW_SOCKET_ERROR:
	// 	prvlwip.socket_busy &= ~(1 << param1);
	// 	break;
	// }
    // LLOGD("=================================================");
    // LLOGD("event %08X tag %016X", event.ID, param.tag);
    // LLOGD("event %p %p", &event, &param);
	ctrl.socket_cb(&event, &param);
    // LLOGD("+++++++++++++++++++++++++++++++++++++++++++++++++");
}

// static int network_state = 0;
static int libuv_set_dns_server(uint8_t server_index, luat_ip_addr_t *ip, void *user_data);


static void libuv_callback_to_nw_task(uint8_t adapter_index, uint32_t event_id, uint32_t param1, uint32_t param2, uint32_t param3);


static int libuv_socket_check(int socket_id, uint64_t tag, void *user_data)
{
    if (socket_id < 0 || socket_id >= MAX_SOCK_NUM)
        return -1;
    if (sockets[socket_id].tag == tag)
        return 0;
	return -1;
}


static uint8_t libuv_check_ready(void *user_data)
{
	return 1; // 当前总是当成联网状态
}

static int libuv_create_socket(uint8_t is_tcp, uint64_t *tag, void *param, uint8_t is_ipv6, void *user_data)
{
	// LLOGD("执行libuv_create_socket");
    uint64_t stag = socket_tag_counter;
    socket_tag_counter++;
    for (size_t i = 0; i < MAX_SOCK_NUM; i++)
    {
        if (sockets[i].tag == 0) {
            uv_tcp_init(main_loop, &sockets[i].tcp);
            *tag = stag;
            sockets[i].tag = stag;
            sockets[i].param = param;
            sockets[i].is_tcp = is_tcp;
            sockets[i].is_ipv6 = is_ipv6;
            return i;
        }
    }
    LLOGE("too many socket created");
	return -1;
}
static void buf_alloc(uv_handle_t* tcp, size_t size, uv_buf_t* buf) {
    // LLOGD("buf_alloc %d", size);
    void* ptr = luat_heap_malloc(size);
    buf->len = ptr == NULL ? 0 : size;
    buf->base = ptr;
}

static void on_recv(uv_stream_t* tcp,
                           ssize_t nread,
                           const uv_buf_t* buf) {
    int32_t socket_id = (int32_t)tcp->data;
    // LLOGD("on_recv %d %d", socket_id, nread);
    if (nread < 0) {
        luat_heap_free(buf->base);
        if (nread == UV_EOF) {
            // LLOGD("服务器断开了连接");
            cb_to_nw_task(EV_NW_SOCKET_REMOTE_CLOSE, socket_id, 0, sockets[socket_id].param);
        }
        else {
            LLOGD("socket read出错, 被其他原因中断了?");
            cb_to_nw_task(EV_NW_SOCKET_ERROR, socket_id, 0, sockets[socket_id].param);
        }
        // uv_close((uv_handle_t*) tcp, NULL);
        return;
    }
    else if (nread > 0) {
        // LLOGD("on_recv 待读取数据长度 %d", nread);
        // LLOGD("待读取内容 %.*s", nread, buf->base);
        if (sockets[socket_id].recv_buff == NULL) {
            sockets[socket_id].recv_buff = luat_heap_malloc(nread);
            sockets[socket_id].recv_size = nread;
            memcpy(sockets[socket_id].recv_buff, buf->base, nread);
        }
        else {
            void* ptr = luat_heap_realloc(sockets[socket_id].recv_buff, sockets[socket_id].recv_size + nread);
            if (ptr == NULL) {
                LLOGD("内存不足, 无法存放更多接收到的数据");
                cb_to_nw_task(EV_NW_SOCKET_ERROR, socket_id, 0, sockets[socket_id].param);
                return;
            }
            sockets[socket_id].recv_buff = ptr;
            memcpy(sockets[socket_id].recv_buff + sockets[socket_id].recv_size, buf->base, nread);
            sockets[socket_id].recv_size += nread;
        }
        luat_heap_free(buf->base);
        cb_to_nw_task(EV_NW_SOCKET_RX_NEW, socket_id, nread, sockets[socket_id].param);
        return;
    }
}

static void on_connect(uv_connect_t* req, int status) {
    // LLOGD("on_connect %d", status);
    int32_t socket_id = (int32_t)req->data;
    if (status != 0) {
        LLOGE("连接服务器失败");
        cb_to_nw_task(EV_NW_SOCKET_ERROR, socket_id, 0, sockets[socket_id].param);
    }
    else {
        cb_to_nw_task(EV_NW_SOCKET_CONNECT_OK, socket_id, 0, sockets[socket_id].param);
    }

    if (status == 0) {
        // LLOGD("启动接收回调");
        int ret = uv_read_start(&sockets[socket_id].tcp, buf_alloc, on_recv);
        if (ret) // TODO 中止连接
            LLOGD("uv_read_start %d", ret);
    }
    else {
        LLOGD("连接失败了,无需启动接收回调啦");
    }
}

//作为client绑定一个port，并连接remote_ip和remote_port对应的server
static int libuv_socket_connect(int socket_id, uint64_t tag,  uint16_t local_port, luat_ip_addr_t *remote_ip, uint16_t remote_port, void *user_data)
{
    CHECK_SOCKET_ID

    struct sockaddr_in saddr = {
        .sin_family = AF_INET
    };
    saddr.sin_addr.s_addr = remote_ip->ipv4;
    saddr.sin_port = htons(remote_port);
    char addr[17] = {'\0'};
    uv_ip4_name(&saddr, addr, 16);
    LLOGD("connect to %s:%d", addr, remote_port);
    sockets[socket_id].c.data = (void*)socket_id;
    int ret = uv_tcp_connect(&sockets[socket_id].c, &sockets[socket_id].tcp, (const struct sockaddr*)&saddr, on_connect);
	if (ret)
        LLOGD("uv_tcp_connect ret %d", ret);
    return ret;
}
//作为server绑定一个port，开始监听
static int libuv_socket_listen(int socket_id, uint64_t tag,  uint16_t local_port, void *user_data)
{
    LLOGD("执行listen, 未支持");
	return -1;
}
//作为server接受一个client
static int libuv_socket_accept(int socket_id, uint64_t tag,  luat_ip_addr_t *remote_ip, uint16_t *remote_port, void *user_data)
{
    LLOGD("执行accept, 未支持");
	return -1;
}

static void on_close(uv_handle_t* handle) {
    int32_t socket_id = (int32_t)handle->data;
    // LLOGD("on_close %d", socket_id);
    if (socket_id < 0 || socket_id >= MAX_SOCK_NUM) {
        return;
    }
    if (sockets[socket_id].tag == 0) {
        LLOGD("已经关闭过了 %d", socket_id);
        return;
    }
    cb_to_nw_task(EV_NW_SOCKET_CLOSE_OK, socket_id, 0, sockets[socket_id].param);
    sockets[socket_id].tag = 0;
}

//主动断开一个tcp连接，需要走完整个tcp流程，用户需要接收到close ok回调才能确认彻底断开
static int libuv_socket_disconnect(int socket_id, uint64_t tag,  void *user_data)
{
    CHECK_SOCKET_ID

    // LLOGD("disconnect %d %08X", socket_id, tag);
    // LLOGD("disconnect2 %d %08X", socket_id, sockets[socket_id].tag);
    int ret = uv_tcp_close_reset(&sockets[socket_id].tcp, on_close);
    if (ret)
        LLOGD("disconnect tcp_close_reset %d", ret);
	return 0;
}

static int libuv_socket_force_close(int socket_id, void *user_data)
{
    if (socket_id < 0 || socket_id >= MAX_SOCK_NUM) {
        LLOGE("socket id不合法 %d", socket_id);
        return -1;
    }

    // LLOGD("CALL libuv_socket_force_close %d", socket_id);

    if (sockets[socket_id].tag == 0) {
        LLOGD("该socket已经释放,无需再次释放");
        return 0;
    }

    int ret = uv_tcp_close_reset(&sockets[socket_id].tcp, on_close);
    if (ret)
        LLOGD("force_close tcp_close_reset %d", ret);
    sockets[socket_id].tag = 0;
	return 0;
}

static int libuv_socket_close(int socket_id, uint64_t tag,  void *user_data)
{
    CHECK_SOCKET_ID

    // LLOGD("close %d %016X", socket_id, tag);
    // LLOGD("close2 %d %016X", socket_id, sockets[socket_id].tag);
    int ret = uv_tcp_close_reset(&sockets[socket_id].tcp, on_close);
    if (ret)
        LLOGD("close tcp_close_reset %d", ret);
	return ret;
}

static int libuv_socket_receive(int socket_id, uint64_t tag,  uint8_t *buf, uint32_t len, int flags, luat_ip_addr_t *remote_ip, uint16_t *remote_port, void *user_data)
{
    CHECK_SOCKET_ID

    // LLOGD("socket_receive %d %p %d", socket_id, buf, len);
    if (buf == NULL) {
        return sockets[socket_id].recv_size;
    }

    if (sockets[socket_id].recv_size == 0 && len > 0) {
        LLOGD("无数据可读 expect %d but %d", len, sockets[socket_id].recv_size);
        return 0;
    }
    if (len > sockets[socket_id].recv_size) {
        len = sockets[socket_id].recv_size;
    }
    memcpy(buf, sockets[socket_id].recv_buff, len);
    size_t newsize = sockets[socket_id].recv_size - len;
    if (newsize == NULL) {
        luat_heap_free(sockets[socket_id].recv_buff);
        sockets[socket_id].recv_buff = NULL;
        sockets[socket_id].recv_size = 0;
    }
    else {
        void* ptr = luat_heap_malloc(newsize);
        memcpy(ptr, sockets[socket_id].recv_buff + len, newsize);
        
        luat_heap_free(sockets[socket_id].recv_buff);
        sockets[socket_id].recv_buff = ptr;
        sockets[socket_id].recv_size = newsize;
    }
    // LLOGD("返回数据长度 %d", len);
	return len;
}

static void on_sent(uv_write_t* req, int status) {
    char* tmp = (char*)req;
    tmp += sizeof(uv_write_t);
    uint32_t len = 0;
    memcpy(&len, tmp, 4);
    int socket_id = (int32_t)req->data;
    LLOGD("socket_id sent %d %d %d", socket_id, status, len);

    if (status == 0) {
        // LLOGD("发送成功, 执行TX_OK消息");
        cb_to_nw_task(EV_NW_SOCKET_TX_OK, socket_id, len, sockets[socket_id].param);
    }
    else {
        // LLOGD("发送成功, 执行ERROR消息");
        cb_to_nw_task(EV_NW_SOCKET_ERROR, socket_id, 0, sockets[socket_id].param);
    }
}

static int libuv_socket_send(int socket_id, uint64_t tag, const uint8_t *buf, uint32_t len, int flags, luat_ip_addr_t *remote_ip, uint16_t remote_port, void *user_data)
{
    CHECK_SOCKET_ID

	uv_write_t* req;
    uv_buf_t buff;

    if (len == 0)
        return 0;

    buff = uv_buf_init(buf, len);
    // LLOGD("待发送的内容 %.*s", len, buf);
    req = luat_heap_malloc(sizeof(uv_write_t) + 4);
    memset(req, 0, sizeof(uv_write_t));
    char* tmp = (char*)req;
    tmp += sizeof(uv_write_t);
    memcpy(tmp, &len, 4);
    req->data = (void*)socket_id;
    int ret = uv_write(req, (uv_stream_t*) &sockets[socket_id].tcp, &buff, 1, on_sent);
    // LLOGD("uv_write %d", ret);
    if (ret) {
        luat_heap_free(req);
        return -1;
    }
    return len;
}

void libuv_socket_clean(int *vaild_socket_list, uint32_t num, void *user_data)
{
    // LLOGD("CALL socket clean %p %d", vaild_socket_list, num);
    for (size_t i = 0; i < num; i++)
    {
        int socket_id = vaild_socket_list[i];
        if (socket_id < 0 || socket_id >= MAX_SOCK_NUM)
            continue;
        if (sockets[socket_id].tag == 0) {
            if (sockets[socket_id].recv_buff != NULL) {
                luat_heap_free(sockets[socket_id].recv_buff);
                sockets[socket_id].recv_buff = NULL;
            }
            sockets[socket_id].recv_size = 0;
            continue;
        }
    }
	return 0;
}


static int libuv_get_local_ip_info(luat_ip_addr_t *ip, luat_ip_addr_t *submask, luat_ip_addr_t *gateway, void *user_data)
{
    LLOGD("获取本地IP信息, 未实现");
	return -1;
}

static int libuv_get_full_ip_info(luat_ip_addr_t *ip, luat_ip_addr_t *submask, luat_ip_addr_t *gateway, luat_ip_addr_t *ipv6, void *user_data)
{
    LLOGD("获取全部本地IP信息, 未实现");
	return -1;
}

static int libuv_user_cmd(int socket_id, uint64_t tag, uint32_t cmd, uint32_t value, void *user_data)
{
    // LLOGD("libuv_user_cmd, 未实现");
	return 0;
}

static void on_resolved(uv_getaddrinfo_t *resolver, int status, struct addrinfo *res) {
    uv_dns_query_t* query = resolver->data;
    // LLOGD("dns result %d %p", status, query);
    if (status < 0) {
        LLOGD("dns query failed");
        cb_to_nw_task(EV_NW_DNS_RESULT, 0, 0, query->param);
        luat_heap_free(query);
        return;
    }
    char addr[17] = {'\0'};
    uv_ip4_name((struct sockaddr_in*) res->ai_addr, addr, 16);
    LLOGD("dns result ip %s", addr);
    
    luat_dns_ip_result *ip_result = zalloc(sizeof(luat_dns_ip_result));
    ip_result->ip.ipv4 = ((struct sockaddr_in*) res->ai_addr)->sin_addr.s_addr;
    ip_result->ttl_end = 60;
    cb_to_nw_task(EV_NW_DNS_RESULT, 1, (int)ip_result, query->param);
    luat_heap_free(query);
    uv_freeaddrinfo(res);
}

static int libuv_dns(const char *domain_name, uint32_t len, void *param, void *user_data)
{
	// LLOGD("执行libuv_dns %.*s %p", len, domain_name, param);
    uv_dns_query_t* query = luat_heap_zalloc(sizeof(uv_dns_query_t));
    if (query == NULL) {
        LLOGE("out of memory when malloc dns query");
        return -1;
    }
    memcpy(query->domain, domain_name, len);
    query->param = param;
    // query->user_data = user_data;
    // LLOGD("query %p", query);

    query->hints.ai_family = PF_INET;
    query->hints.ai_socktype = SOCK_STREAM;
    query->hints.ai_protocol = IPPROTO_TCP;
    query->hints.ai_flags = 0;
    query->resolver.data = query;

    int r = uv_getaddrinfo(main_loop, &query->resolver, on_resolved, query->domain, NULL, &query->hints);
    
    if (r != 0) {
        LLOGD("uv_getaddrinfo %d", r);
        luat_heap_free(query);
        cb_to_nw_task(EV_NW_DNS_RESULT, 0, 0, param);
    }
	return r;
}

static int libuv_dns_ipv6(const char *domain_name, uint32_t len, void *param, void *user_data)
{
	LLOGD("执行libuv_dns_ipv6, 未实现");
	// char* ptr = luat_heap_malloc(len + 1);
	// memcpy(ptr, domain_name, len);
	// ptr[len] = 0x00;
	// libuv_send_event(EV_LIBUV_SOCKET_DNS_IPV6, ptr, 0); // TODO 检查返回值
	return -1;
}

static int libuv_set_dns_server(uint8_t server_index, luat_ip_addr_t *ip, void *user_data)
{
	return 0;
}

static int libuv_set_mac(uint8_t *mac, void *user_data)
{
	return -1;
}
int libuv_set_static_ip(luat_ip_addr_t *ip, luat_ip_addr_t *submask, luat_ip_addr_t *gateway, luat_ip_addr_t *ipv6, void *user_data)
{
	return 0;
}

static int32_t libuv_dummy_callback(void *pData, void *pParam)
{
	return 0;
}

static void libuv_socket_set_callback(CBFuncEx_t cb_fun, void *param, void *user_data)
{
	// LLOGD("执行libuv_socket_set_callback %p %p", cb_fun, user_data);
	ctrl.socket_cb = cb_fun?cb_fun:libuv_dummy_callback;
	ctrl.user_data = param;
}

int libuv_getsockopt2(int socket_id, uint64_t tag,  int level, int optname, void *optval, uint32_t *optlen, void *user_data) {
	LLOGD("not support yet: getsockopt");
	return 0;
}

int libuv_setsockopt2(int socket_id, uint64_t tag,  int level, int optname, const void *optval, uint32_t optlen, void *user_data) {
	LLOGD("not support yet: setsockopt");
	return 0;
}

static const network_adapter_info prv_libuv_adapter =
{
		.check_ready = libuv_check_ready,
		.create_soceket = libuv_create_socket,
		.socket_connect = libuv_socket_connect,
		.socket_listen = libuv_socket_listen,
		.socket_accept = libuv_socket_accept,
		.socket_disconnect = libuv_socket_disconnect,
		.socket_close = libuv_socket_close,
		.socket_force_close = libuv_socket_force_close,
		.socket_receive = libuv_socket_receive,
		.socket_send = libuv_socket_send,
		.socket_check = libuv_socket_check,
		.socket_clean = libuv_socket_clean,
		.getsockopt = libuv_getsockopt2,
		.setsockopt = libuv_setsockopt2,
		.user_cmd = libuv_user_cmd,
		.dns = libuv_dns,
		.set_dns_server = libuv_set_dns_server,
		.dns_ipv6 = libuv_dns_ipv6,
		.set_mac = libuv_set_mac,
		.set_static_ip = libuv_set_static_ip,
		.get_local_ip_info = libuv_get_local_ip_info,
		.get_full_ip_info = libuv_get_full_ip_info,
		.socket_set_callback = libuv_socket_set_callback,
		.name = "libuv",
		.max_socket_num = MAX_SOCK_NUM,
		.no_accept = 1,
		.is_posix = 0,
};

void luat_network_init(void) {
    network_register_adapter(NW_ADAPTER_INDEX_ETH0, &prv_libuv_adapter, NULL);
}
