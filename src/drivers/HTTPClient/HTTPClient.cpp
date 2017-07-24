#include "HTTPClient.h"

static char tag[] = "HTTPClient";

HTTPClient_t::HTTPClient_t() {
  SocketID        = -1;
  HTTPRequest[128]= {0};
  BufferSize      = 1024;

  Method          = "GET";

  Task            = new HTTPClientTask_t();
}

void HTTPClient_t::Request() {
  ESP_LOGD(tag, "Start Query");
  ESP_LOGI(tag, "Server IP: %s Server Port:%s", IP.c_str(), Port.c_str());

  sprintf(HTTPRequest, "%s %s HTTP/1.1\r\nHost: %s:%s \r\n\r\n", Method.c_str(), ContentURI.c_str(), Hostname.c_str(), Port.c_str());

  Task = new HTTPClientTask_t();
  Task->SetStackSize(8096);
  Task->Start((void *)this);
}

void HTTPClientTask_t::Run(void *data) {
  HTTPClient_t* HTTPClient = (HTTPClient_t*)data;

  /*connect to http server*/
  if (HTTPClient->HttpConnect()) {
      ESP_LOGI(tag, "Connected to http server");
  }
  else {
      ESP_LOGE(tag, "Connect to http server failed!");
      HTTPClient->task_fatal_error();
  }

  int res = -1;

  // send request to http server
  res = send(HTTPClient->SocketID, HTTPClient->HTTPRequest, strlen(HTTPClient->HTTPRequest), 0);
  if (res == -1) {
      ESP_LOGE(tag, "Send GET request to server failed");
      HTTPClient->task_fatal_error();
  }
  else
      ESP_LOGI(tag, "Send GET request to server succeeded");

  bool resp_body_start = false, flag = true;
  char ReadData[HTTPClient->BufferSize + 1] = { 0 };
  char Text[HTTPClient->BufferSize + 1]     = { 0 };

  HTTPClient->ReadStarted();

  while (flag) {
    memset(Text, 0, HTTPClient->BufferSize);
    memset(ReadData, 0, HTTPClient->BufferSize);

    int BuffLen = recv(HTTPClient->SocketID, Text, HTTPClient->BufferSize, 0);

    if (BuffLen < 0) {
      ESP_LOGE(tag, "Error: receive data error! errno=%d", errno);
      HTTPClient->task_fatal_error();
    }
    else if (BuffLen > 0 && !resp_body_start) { // skip header
      memcpy(ReadData, Text, BuffLen);
      resp_body_start = HTTPClient->ReadPastHttpHeader(Text, BuffLen);
    }
    else if (BuffLen > 0 && resp_body_start) {
      memcpy(ReadData, Text, BuffLen);
      HTTPClient->ReadBody(ReadData, BuffLen);
    }
    else if (BuffLen == 0) {
      /* packet over */
      flag = false;
      ESP_LOGI(tag, "Connection closed, all packets received");
      close(HTTPClient->SocketID);
    }
    else
      ESP_LOGE(tag, "Unexpected recv result");
  }

  if (!HTTPClient->ReadFinished())
    HTTPClient->task_fatal_error();

  Stop();

  return ;
}

bool HTTPClient_t::HttpConnect() {
  int  http_connect_flag = -1;
  struct sockaddr_in sock_info;

  SocketID = socket(AF_INET, SOCK_STREAM, 0);

  if (SocketID == -1) {
      ESP_LOGE(tag, "Create socket failed!");
      return false;
  }

  // set connect info
  memset(&sock_info, 0, sizeof(struct sockaddr_in));
  sock_info.sin_family = AF_INET;
  sock_info.sin_addr.s_addr = inet_addr(IP.c_str());
  sock_info.sin_port = htons(atoi(Port.c_str()));

  // connect to http server
  http_connect_flag = connect(SocketID, (struct sockaddr *)&sock_info, sizeof(sock_info));
  if (http_connect_flag == -1)
  {
      ESP_LOGE(tag, "Connect to server failed! errno=%d", errno);
      close(SocketID);
      return false;
  }
  else
  {
      ESP_LOGI(tag, "Connected to server");
      return true;
  }
  return false;
}

// read buffer by byte still delim ,return read bytes counts
int HTTPClient_t::ReadUntil(char *buffer, char delim, int len) {
    /* TODO: delim check,buffer check,further: do an buffer length limited */
    int i = 0;
    while (buffer[i] != delim && i < len) {
        ++i;
    }
    return i + 1;
}

/* resolve a packet from http socket
 * return true if packet including \r\n\r\n that means http packet header finished,start to receive packet body
 * otherwise return false
 * */
bool HTTPClient_t::ReadPastHttpHeader(char text[], int total_len) {
    /* i means current position */
    int i = 0, i_read_len = 0;
    char ReadData[BufferSize + 1] = { 0 };

    while (text[i] != 0 && i < total_len)
    {
        i_read_len = ReadUntil(&text[i], '\n', total_len);
        // if we resolve \r\n line,we think packet header is finished

        if (i_read_len == 2)
        {
            int i_write_len = total_len - (i + 2);

            memset(ReadData, 0, BufferSize);
            memcpy(ReadData, &(text[i + 2]), i_write_len);

            if (!ReadBody(ReadData,i_write_len))
              task_fatal_error();

            return true;
        }
        i += i_read_len;
    }

    return false;
}

void __attribute__((noreturn)) HTTPClient_t::task_fatal_error()
{
    ESP_LOGE(tag, "Exiting HTTPClient task due to fatal error...");
    close(SocketID);
    Task->Stop();

    Aborted();

    while (1) {
        ;
    }
}
