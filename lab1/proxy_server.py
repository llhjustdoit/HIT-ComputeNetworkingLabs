# -*- coding:utf-8 -*- 

import socket
import thread  
import urlparse  
import select  
  
BUFLEN = 8192

fish_header = """
GET http://software.hit.edu.cn/ HTTP/1.1\n
Host: software.hit.edu.cn\n
Proxy-Connection: keep-alive\n
Upgrade-Insecure-Requests: 1\n
User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/52.0.2743.116 Safari/537.36\n
Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8\n
Accept-Encoding: gzip, deflate, sdch\n
Accept-Language: zh-CN,zh;q=0.8\n\n
"""
  
class Proxy(object):  
    def __init__(self, conn, addr):
        self.source = conn
        self.request = ""
        self.headers = {}    # 头信息，包括方法，路径，协议版本
		# 新建目的套接字，socket.AF_INET为协议族，socket.SOCK_STREAM指定套接字类型为流
        self.destnation = socket.socket(socket.AF_INET, socket.SOCK_STREAM) 
        self.run()
  
    def get_headers(self):  
        header = ''
        while True:  
            header += self.source.recv(BUFLEN)
            index = header.find('\n')  # 第一个换行符的索引
            if index > 0:
                break
			# else 请求头为空，重复获取
        firstLine = header[:index] # 获取请求头第一行，包括方法，url，协议版本
        self.request = header[index+1:] 
        self.headers['method'], self.headers['path'], self.headers['protocol'] = firstLine.split()

    def conn_destnation(self):  
        url = urlparse.urlparse(self.headers['path'])  # 从url string中获取url（元组类型）
        hostname = url[1]
        port = "80"  # 默认端口为80
        if hostname.find(':') > 0:
            addr, port = hostname.split(':')
        else:  
            addr = hostname
        port = int(port)
        ip = socket.gethostbyname(addr)  # 根据主机名获取ip地址
        try:
            self.destnation.connect((ip, port)) # destnation套接字根据原服务器ip,port建立连接
            data = "%s %s %s\r\n" % (self.headers['method'], self.headers['path'], self.headers['protocol'])
            self.destnation.send(data + self.request)  # 重新构造请求头并发送给原服务器
            print data + self.request
        except:
            pass

    def renderto(self):  
        readsocket = [self.destnation]
        while True:  
            data = ''
            (rlist, wlist, elist) = select.select(readsocket, [], [], 3)
            if rlist:  
                data = rlist[0].recv(BUFLEN)  # 代理服务器从原服务器接受数据
                if len(data) > 0:
                    self.source.send(data)    # 向客户端发送数据
                else:
                    break  
    def run(self):  
        self.get_headers()  
        self.conn_destnation()  
        self.renderto()
  

class Server(object):  
  
    def __init__(self, host, port, handler=Proxy):
        self.host = host
        self.port = port
        self.server = socket.socket(socket.AF_INET, socket.SOCK_STREAM) # 代理服务器主套接字
        self.server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)  # 打开地址复用功能
        self.server.bind((host, port))  # 绑定代理服务器套接字的本地断点地址
        self.server.listen(5)  # 至代理服务器套接字为监听状态且队列大小为5
        self.handler = handler
  
    def start(self):  
        while True:  
            try:  
                conn, addr = self.server.accept()   # 接受一个连接请求
				# 创建代理线程，即每针对一个接受的连接请求创建子线程实现一对一代理
                thread.start_new_thread(self.handler, (conn, addr))  
            except:  
                pass
  

if __name__ == '__main__':
    host = '127.0.0.1'
    port = 8080
    s = Server(host, port)
    print "host: %s, port: %d" % (host, port)
    print 'running...'
    s.start()
