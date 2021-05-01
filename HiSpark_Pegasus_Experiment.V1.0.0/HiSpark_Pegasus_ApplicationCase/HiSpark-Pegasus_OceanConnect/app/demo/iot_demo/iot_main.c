/*
 * Copyright (c) 2020 HiHope Community.
 * Description: IoT platform
 * Author: HiSpark Product Team
 * Create: 2020-5-20
 */

/* we use the mqtt to connect to the huawei IoT platform*/
/**
 * STEPS:
 * 1, CONNECT TO THE IOT SERVER
 * 2, SUBSCRIBE  THE DEFAULT TOPIC
 * 3, WAIT FOR ANY MESSAGE COMES OR ANY MESSAGE TO SEND
 * 
*/
#include "iot_config.h"
#include "iot_log.h"
#include "iot_main.h"
#include "iot_hmac.h"
#include <securec.h>
#include <hi_task.h>
#include <hi_msg.h>
#include <hi_mem.h>
#include <MQTTClient.h>
#include <string.h>

///< this is the configuration head
#ifdef CONFIG_MQTT_SSL
#define CN_IOT_SERVER     "ssl://121.36.42.100:8883"
static const char gIotCA[] =
" -----BEGIN CERTIFICATE-----\r\n"
"MIIDrzCCApegAwIBAgIQCDvgVpBCRrGhdWrJWZHHSjANBgkqhkiG9w0BAQUFADBh\r\n"
"MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\r\n"
"d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBD\r\n"
"QTAeFw0wNjExMTAwMDAwMDBaFw0zMTExMTAwMDAwMDBaMGExCzAJBgNVBAYTAlVT\r\n"
"MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j\r\n"
"b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IENBMIIBIjANBgkqhkiG\r\n"
"9w0BAQEFAAOCAQ8AMIIBCgKCAQEA4jvhEXLeqKTTo1eqUKKPC3eQyaKl7hLOllsB\r\n"
"CSDMAZOnTjC3U/dDxGkAV53ijSLdhwZAAIEJzs4bg7/fzTtxRuLWZscFs3YnFo97\r\n"
"nh6Vfe63SKMI2tavegw5BmV/Sl0fvBf4q77uKNd0f3p4mVmFaG5cIzJLv07A6Fpt\r\n"
"43C/dxC//AH2hdmoRBBYMql1GNXRor5H4idq9Joz+EkIYIvUX7Q6hL+hqkpMfT7P\r\n"
"T19sdl6gSzeRntwi5m3OFBqOasv+zbMUZBfHWymeMr/y7vrTC0LUq7dBMtoM1O/4\r\n"
"gdW7jVg/tRvoSSiicNoxBN33shbyTApOB6jtSj1etX+jkMOvJwIDAQABo2MwYTAO\r\n"
"BgNVHQ8BAf8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4EFgQUA95QNVbR\r\n"
"TLtm8KPiGxvDl7I90VUwHwYDVR0jBBgwFoAUA95QNVbRTLtm8KPiGxvDl7I90VUw\r\n"
"DQYJKoZIhvcNAQEFBQADggEBAMucN6pIExIK+t1EnE9SsPTfrgT1eXkIoyQY/Esr\r\n"
"hMAtudXH/vTBH1jLuG2cenTnmCmrEbXjcKChzUyImZOMkXDiqw8cvpOp/2PV5Adg\r\n"
"06O/nVsJ8dWO41P0jmP6P6fbtGbfYmbW0W5BjfIttep3Sp+dWOIrWcBAI+0tKIJF\r\n"
"PnlUkiaY4IBIqDfv8NZ5YBberOgOzW6sRBc4L0na4UU+Krk2U886UAb3LujEV0ls\r\n"
"YSEY1QSteDwsOoBrp+uvFRTp2InBuThs4pFsiv9kuXclVzDAGySj4dzp30d8tbQk\r\n"
"CAUw7C29C79Fv1C5qfPrmAESrciIxpg0X40KPMbp1ZWVbd4=\r\n"
"-----END CERTIFICATE-----\r\n";

#ifdef CONFIG_MQTT_SSL_X509
static const char gDeviceCA[] = \
"-----BEGIN CERTIFICATE-----\r\n"
"MIICuDCCAaACCQDAnCoP3W+otTANBgkqhkiG9w0BAQsFADAdMQswCQYDVQQGEwJD\r\n"
"QTEOMAwGA1UEAwwFTVkgQ0EwHhcNMjAwMjE3MTEyNTA2WhcNMjEwMjExMTEyNTA2\r\n"
"WjAfMQswCQYDVQQGEwJDTjEQMA4GA1UEAwwHQ0xJRU5UMjCCASIwDQYJKoZIhvcN\r\n"
"AQEBBQADggEPADCCAQoCggEBALKOOfeq1QQnwLNsMboqr47ClO24sTd6oLiSjGYe\r\n"
"zb7IECnjiEg+sILEH5avi6uKdCR8QZHskIWdQbNqi2h0E01MTxuneTCnFaAdYh2z\r\n"
"B8n4IUsGr1gyCayfKCQG8ElDElBkB2JMce/IEuxJvnFdhXy6c9zhj8iU+fusD9hm\r\n"
"jvx34zd6Sn2uRl8wdI89pVwkESKyx6J5za/dBWCS0hxvOtDEY2gJA9av8BNYjTxQ\r\n"
"tRlwnxGzgaza8aA9LMbnRmv1bML98w4NgNHkVGUtw1eQso0unHnyEF1WhqEu0pLk\r\n"
"BJQbdO4s4ajQ1rOr7niVnCT99KtzJuWm3oymNdDT6BHsnn0CAwEAATANBgkqhkiG\r\n"
"9w0BAQsFAAOCAQEAPayFtS8du8D+mPdRU96vFqVpDx6oksGhrUIO+3GZ4ruuHjtf\r\n"
"A764y6zD1/LxQVw15xN44G+AqUwEi/BZszQ+5rSitwqoZFciKMblaqd0ZJSG6G2d\r\n"
"Cqyl87cTL6Tlgpa5cBka4ksaMs2OVU9x1aaNnejrTbX2PvZO7o/pVVimQE8Yh+1J\r\n"
"k2D26hg9UoWN4Z3/9LdGC3y4dsmtuQS+Cye61ZfHtT3/XjTRjl9/c+xKtVsQpE6g\r\n"
"dpaGVxWpFUCpGe4POrNHjOOAXaFiCHP7o9Kab1UnxuFVKHCtNT9wdJ7v8puCliwB\r\n"
"t40h9oegMyUYL/VG+14wOYgEPuCeGqqilht+4w==\r\n"
"-----END CERTIFICATE-----\r\n";
static const char gDevicePK[]= \
"-----BEGIN PRIVATE KEY-----\r\n"
"MIIEvAIBADANBgkqhkiG9w0BAQEFAASCBKYwggSiAgEAAoIBAQCyjjn3qtUEJ8Cz\r\n"
"bDG6Kq+OwpTtuLE3eqC4koxmHs2+yBAp44hIPrCCxB+Wr4urinQkfEGR7JCFnUGz\r\n"
"aotodBNNTE8bp3kwpxWgHWIdswfJ+CFLBq9YMgmsnygkBvBJQxJQZAdiTHHvyBLs\r\n"
"Sb5xXYV8unPc4Y/IlPn7rA/YZo78d+M3ekp9rkZfMHSPPaVcJBEisseiec2v3QVg\r\n"
"ktIcbzrQxGNoCQPWr/ATWI08ULUZcJ8Rs4Gs2vGgPSzG50Zr9WzC/fMODYDR5FRl\r\n"
"LcNXkLKNLpx58hBdVoahLtKS5ASUG3TuLOGo0Nazq+54lZwk/fSrcyblpt6MpjXQ\r\n"
"0+gR7J59AgMBAAECggEAFbjx2myhD6laazmdbx2kUATiI22Vub8kcG/7vqy+sRgb\r\n"
"x0zUFp3x0WbUxPtETbFBYfLv8zXJe0ffZ1lt7l44vu2qrh2Yw89/9QO+HQjbyLl0\r\n"
"K3F7wYoM3UBbnM5vSypqvvb8YvwT3G0FyFNMZPRQuhTg5Z1XZbqiEvki0Fvhx1sG\r\n"
"bksCKK85dPaH/7kXxMTPZ67/2Z3TlwUERo1YSPRnrQXGnDowudLqQjn8ptiPfO39\r\n"
"bIMQhM/HI1bTw4F5EevL7sS4rcnhpmQNU6ZeINPvYuel+hP36uPwTHMm1hkZbMgP\r\n"
"a69Yk8/meENffl9fQ+T8woYMGgKq0DM+O5ip7QzrLQKBgQDdvwI/9npuwpYkdjWT\r\n"
"tLj5BKFNINjn+TjyvN1QQmOhXqzTy1y9HVm7EYh1l0H5PY/WgYadBWNYabgNSIe0\r\n"
"vxl7tdDxDX4DBQqDIIEY45sPa3glq6flCgtPzkZxcYTvZpqmdX8/iMYGXHpdOMdm\r\n"
"S8jngobFDEgLxFS1lLLs+LtTTwKBgQDOIz08d2zMRX5AW0SkQ+VbdwcO7G5qlTJV\r\n"
"42Rf51Dmr0as/bX5eIWAAfcsEi/kG2e5Ozupe4BVZXarw7AL1bzE4mLvfcwZEUiJ\r\n"
"Qj56H6QTOiVm/DTZdBz415z60zjCui+59luev6txZBL51ZhoifzfdAfxd8ztucV+\r\n"
"mj3kytIucwKBgBSdporl0norsyPOrap+2dek+44dKK1geU/wCS9mDLc7AxxpwsR+\r\n"
"TxJaTfKIzoRqmc3amvZMQvE8WvU5Mgy225qpy7QtifKj7FdGlVoIBymG11PBQsnD\r\n"
"hku0JM51PGO5iVVChUMV5zs+K84w8jDriNElTHxTaPtXFH0RRxVLBrd3AoGAIYX1\r\n"
"lXnOhcHfcFPna9dm22MWEMU+h7LPxsv+LOKPc5SZp9Ry3alN/K1VJyOmCjCyw5rw\r\n"
"bvWYv0qK7BAc9y/BTTuL32rT4FfBNiFBy9aNdlsEbiUh6yNI+ucFrnzfRDvHOzTn\r\n"
"XX/KP9yIgXzNFmMqbqSODiTvfNGzXo1y91xMpiUCgYAfO+Xb5msOMABiBNGD85qr\r\n"
"C+0xpuWoPe4pB5ccgniv1zhPlUJ+8l1vWz/ZWnH/6oVxGGrq5xLtoBgFWM4uSseI\r\n"
"Je0fSqZp0XAbaFI7wkEV/uP6VMjjFT5TfJz4idgI+EaE72va2jbFBgg1MyfR3gx4\r\n"
"deaXc4ZpE64iV+eAtZ8VVQ==\r\n"
"-----END PRIVATE KEY-----\r\n";
static const char *gDevicePKPwd = "123456";
#endif  ///< end for x509
#else
#define CN_IOT_SERVER     "tcp://121.36.42.100:1883"
#endif  ///< end for ssl

#define CONFIG_COMMAND_TIMEOUT     10000L
#define CN_KEEPALIVE_TIME 50
#define CN_CLEANSESSION   1
#define CN_HMAC_PWD_LEN   65     ///< SHA256 IS 32 BYTES AND END APPEND '\0'
#define CN_EVENT_TIME     "1970000100"
#define CN_CLIENTID_FMT   "%s_0_0_%s"      ///< This is the cient ID format, deviceID_0_0_TIME
#define CN_QUEUE_WAITTIMEOUT   1000
#define CN_QUEUE_MSGNUM 16
#define CN_QUEUE_MSGSIZE (sizeof(hi_pvoid))

#define CN_TASK_PRIOR 28
#define CN_TASK_STACKSIZE 0X2000
#define CN_TASK_NAME "IoTMain"

typedef enum{
    EN_IOT_MSG_PUBLISH = 0,
    EN_IOT_MSG_RECV,
}en_iot_msg_t;

typedef struct{
    en_iot_msg_t type;
    int qos;
    const char *topic;
    const char *payload;
}IoTMsg_t;

typedef struct 
{   
    hi_bool  stop;
    hi_u32 conLost;
    hi_u32 queueID;
    hi_u32 iotTaskID;
    fnMsgCallBack msgCallBack;
    MQTTClient_deliveryToken tocken;
}IotAppCb_t;
static IotAppCb_t   gIoTAppCb;

static const char *gDefaultSubscribeTopic[] ={
    "$oc/devices/"CONFIG_DEVICE_ID"/sys/messages/down",
    "$oc/devices/"CONFIG_DEVICE_ID"/sys/properties/set/#",
    "$oc/devices/"CONFIG_DEVICE_ID"/sys/properties/get/#",
    "$oc/devices/"CONFIG_DEVICE_ID"/sys/shadow/get/response/#",
    "$oc/devices/"CONFIG_DEVICE_ID"/sys/events/down",
    "$oc/devices/"CONFIG_DEVICE_ID"/sys/commands/#"
};
#define CN_TOPIC_SUBSCRIBE_NUM     (sizeof(gDefaultSubscribeTopic)/sizeof( const char *))

static int MsgRcvCallBack(void *context, char *topic, int topicLen, MQTTClient_message *message)
{
    IoTMsg_t  *msg;
    char *buf;
    hi_u32 bufSize;

    if(topicLen == 0){
        topicLen = strlen(topic);
    }
    bufSize = topicLen + 1  + message->payloadlen + 1 + sizeof(IoTMsg_t);
    buf = hi_malloc(0,bufSize);
    if(buf != NULL){
        msg = (IoTMsg_t *)buf;
        buf += sizeof(IoTMsg_t);
        bufSize -= sizeof(IoTMsg_t);
        msg->qos = message->qos;
        msg->type = EN_IOT_MSG_RECV;
        (void)memcpy_s(buf,bufSize,topic, topicLen);
        buf[topicLen] = '\0';
        msg->topic = buf;
        buf += topicLen + 1;
        bufSize -= (topicLen + 1);
        (void)memcpy_s(buf,bufSize,message->payload, message->payloadlen);
        buf[message->payloadlen] = '\0';
        msg->payload = buf;
        IOT_LOG_DEBUG("RCVMSG:QOS:%d TOPIC:%s PAYLOAD:%s\r\n",msg->qos,msg->topic,msg->payload);
        if(HI_ERR_SUCCESS != hi_msg_queue_send(gIoTAppCb.queueID,&msg,CN_QUEUE_WAITTIMEOUT,sizeof(hi_pvoid))){
            IOT_LOG_ERROR("Wrie queue failed\r\n");
            hi_free(0, msg);
        }
    }

    MQTTClient_freeMessage(&message);
    MQTTClient_free(topic);
    return 1;
}

///< when the connect lost and this callback will be called
static void ConnLostCallBack(void *context, char *cause)
{
    IOT_LOG_DEBUG("Connection lost:caused by:%s\r\n",cause == NULL? "Unknown":cause);
    return;
}

///<use this function to deal all the comming message
static int ProcessQueueMsg(MQTTClient client)
{
    hi_u32     ret;
    hi_u32     msgSize;
    IoTMsg_t  *msg;
    hi_u32     timeout;
    MQTTClient_message pubmsg = MQTTClient_message_initializer;

    timeout = CN_QUEUE_WAITTIMEOUT;
    do{
        msg = NULL;
        msgSize = sizeof(hi_pvoid);
        ret = hi_msg_queue_wait(gIoTAppCb.queueID, &msg, timeout, &msgSize);
        if(msg != NULL) {
            // IOT_LOG_DEBUG("QUEUEMSG:QOS:%d TOPIC:%s PAYLOAD:%s\r\n",msg->qos,msg->topic,msg->payload);
            switch (msg->type)
            {
                case EN_IOT_MSG_PUBLISH:
                    pubmsg.payload = (void *)msg->payload;
                    pubmsg.payloadlen = (int)strlen(msg->payload);
                    pubmsg.qos = msg->qos;
                    pubmsg.retained = 0;
                    ret = MQTTClient_publishMessage(client, msg->topic, &pubmsg, &gIoTAppCb.tocken);
                    if(ret != MQTTCLIENT_SUCCESS){
                        IOT_LOG_ERROR("MSGSEND:failed\r\n");
                    }
                    IOT_LOG_DEBUG("MSGSEND:SUCCESS\r\n");
                    gIoTAppCb.tocken++;
                    break;
                case EN_IOT_MSG_RECV:
                    if (gIoTAppCb.msgCallBack != NULL)
                    {
                        gIoTAppCb.msgCallBack(msg->qos, msg->topic, msg->payload);  
                    }
                    break;
                default:
                    break;
            }
            hi_free(0,msg);
        }
        timeout = 0;  ///< continous to deal the message without wait here
    } while (ret == HI_ERR_SUCCESS);

    return 0;
}

static hi_void MainEntryProcess(hi_void) 
{
    int  subQos[CN_TOPIC_SUBSCRIBE_NUM] = {1};  
    int rc;
    char *clientID = NULL;
    char *userID = NULL;
    char *userPwd = NULL;

    MQTTClient client;
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
#ifdef CONFIG_MQTT_SSL
    MQTTClient_SSLOptions ssl_opts = MQTTClient_SSLOptions_initializer;
    cert_string trustStore = {(const unsigned char *)gIotCA, sizeof(gIotCA)};
#ifdef CONFIG_MQTT_SSL_X509
    cert_string keyStore = {(const unsigned char *)gDeviceCA, sizeof(gDeviceCA)};
    key_string privateKey = {(const unsigned char *)gDevicePK, sizeof(gDevicePK)};
    ssl_opts.los_keyStore = &keyStore;
    ssl_opts.los_privateKey = &privateKey;
    ssl_opts.privateKeyPassword = gDevicePKPwd;
#endif
    ssl_opts.los_trustStore = &trustStore;  
    conn_opts.ssl = &ssl_opts;
 #endif
    
    IOT_LOG_DEBUG("IoT machine start here\r\n");
    ///< make the clientID userID userPwd
    clientID = hi_malloc(0, strlen(CN_CLIENTID_FMT) +strlen(CONFIG_DEVICE_ID) + strlen(CN_EVENT_TIME) + 1);
    if(NULL== clientID){
       IOT_LOG_ERROR("No memory for the clientID\r\n");
       goto EXIT_CLIENTID;
    }
    (void) snprintf(clientID,strlen(CN_CLIENTID_FMT) +strlen(CONFIG_DEVICE_ID) + strlen(CN_EVENT_TIME) + 1,\
                    CN_CLIENTID_FMT,CONFIG_DEVICE_ID,CN_EVENT_TIME);
    userID = CONFIG_DEVICE_ID;
    if(NULL != CONFIG_DEVICE_PWD){
        userPwd = hi_malloc(0, CN_HMAC_PWD_LEN);
        if(NULL == userPwd){
            IOT_LOG_ERROR("No memory for the user passwd\r\n");
            goto EXIT_USERPWD;
        }
        (void) HmacGeneratePwd((const unsigned char *)CONFIG_DEVICE_PWD,strlen(CONFIG_DEVICE_PWD),\
                                (const unsigned char *)CN_EVENT_TIME,strlen(CN_EVENT_TIME),\
                                (unsigned char *)userPwd, CN_HMAC_PWD_LEN);
    }

    conn_opts.keepAliveInterval = CN_KEEPALIVE_TIME;
    conn_opts.cleansession = CN_CLEANSESSION;
    conn_opts.username = userID;
    conn_opts.password = userPwd;
    conn_opts.MQTTVersion = MQTTVERSION_3_1_1;

    ///< wait for the wifi connect ok
    IOT_LOG_DEBUG("IOTSERVER:%s\r\n",CN_IOT_SERVER);
    IOT_LOG_DEBUG("CLIENTID:%s USERID:%s USERPWD:%s\r\n",clientID, userID,userPwd==NULL?"NULL":userPwd);
    rc = MQTTClient_create(&client, CN_IOT_SERVER, clientID,MQTTCLIENT_PERSISTENCE_NONE, NULL);
    if (rc != MQTTCLIENT_SUCCESS) {
        IOT_LOG_ERROR ("Create Client failed,Please check the parameters--%d\r\n",rc);
        goto EXIT_CREATE;    
    }
    rc = MQTTClient_setCallbacks(client, NULL, ConnLostCallBack, MsgRcvCallBack, NULL);
    if( rc != MQTTCLIENT_SUCCESS){
        IOT_LOG_ERROR ("Set the callback failed,Please check the callback paras\r\n");
        goto EXIT_SETCALLBACK;    
    }
    rc = MQTTClient_connect(client, &conn_opts);
    if (rc != MQTTCLIENT_SUCCESS) {
        IOT_LOG_ERROR ("Connect IoT server failed,please check the network and parameters:%d\r\n",rc);
        goto EXIT_CONNECT;    
    }
    IOT_LOG_DEBUG("Connect success\r\n");

    rc = MQTTClient_subscribeMany(client,CN_TOPIC_SUBSCRIBE_NUM, (char *const *)gDefaultSubscribeTopic, (int *)&subQos[0]);
    if( rc  != MQTTCLIENT_SUCCESS){
        IOT_LOG_ERROR ("Subscribe the default topic failed,Please check the parameters\r\n");
        goto EXIT_SUBSCRIBE;    
    }
    IOT_LOG_DEBUG ("Subscribe success\r\n");

    while (MQTTClient_isConnected(client)){
       
        ProcessQueueMsg(client); ///< do the job here
        MQTTClient_yield();  ///< make the keepalive done
    }
    IOT_LOG_ERROR ("disconnect and begin to quit\r\n");
    MQTTClient_disconnect(client, CONFIG_COMMAND_TIMEOUT);

EXIT_SUBSCRIBE:
EXIT_CONNECT:   
EXIT_SETCALLBACK: 
    MQTTClient_destroy(&client);
EXIT_CREATE:
    if(NULL != userPwd){
        hi_free(0, userPwd);
    }
EXIT_USERPWD:
    hi_free(0,clientID);
EXIT_CLIENTID:
    return;
}

static hi_void *MainEntry(hi_void *arg)
{
    while(gIoTAppCb.stop == HI_FALSE)
    {
        MainEntryProcess();
        IOT_LOG_DEBUG("The connection lost and we will try another connect\r\n");
        hi_sleep(1000);
    }
    return NULL;
} 

int IoTMain(void)
{
    hi_u32 ret;
    hi_task_attr attr = {0};

    ret = hi_msg_queue_create(&gIoTAppCb.queueID, CN_QUEUE_MSGNUM,CN_QUEUE_MSGSIZE);
    if (ret != HI_ERR_SUCCESS) {
       IOT_LOG_ERROR("Create the msg queue Failed\r\n");
    }

    attr.stack_size = CN_TASK_STACKSIZE;
    attr.task_prio = CN_TASK_PRIOR;
    attr.task_name = CN_TASK_NAME;
    ret = hi_task_create(&gIoTAppCb.iotTaskID, &attr, MainEntry, NULL);
    if (ret != HI_ERR_SUCCESS) {
       IOT_LOG_ERROR("Create the Main Entry Failed\r\n");
    }

    return 0;
}

int IoTSetMsgCallback(fnMsgCallBack msgCallback)
{
    gIoTAppCb.msgCallBack = msgCallback;
    return 0;
}

int IotSendMsg(int qos, const char *topic, const char *payload)
{
    int       rc = -1;
    IoTMsg_t  *msg;
    char *buf;
    hi_u32 bufSize;

    bufSize = strlen(topic) + 1 +strlen(payload) + 1 + sizeof(IoTMsg_t);
    buf = hi_malloc(0,bufSize);
    if(buf != NULL){
        msg = (IoTMsg_t *)buf;
        buf += sizeof(IoTMsg_t);
        bufSize -= sizeof(IoTMsg_t);
        msg->qos = qos;
        msg->type = EN_IOT_MSG_PUBLISH;
        (void)memcpy_s(buf,bufSize,topic, strlen(topic));
        buf[strlen(topic)] = '\0';
        msg->topic = buf;
        buf += strlen(topic) + 1;
        bufSize -= (strlen(topic) + 1);
        (void)memcpy_s(buf,bufSize,payload,strlen(payload));
        buf[strlen(payload)] = '\0';
        msg->payload = buf;
        // IOT_LOG_DEBUG("SNDMSG:QOS:%d TOPIC:%s PAYLOAD:%s\r\n",msg->qos,msg->topic,msg->payload);

        if(HI_ERR_SUCCESS != hi_msg_queue_send(gIoTAppCb.queueID,&msg,CN_QUEUE_WAITTIMEOUT,sizeof(hi_pvoid))){
            IOT_LOG_ERROR("Wrie queue failed\r\n");
            hi_free(0, msg);
        }
        else{
            rc = 0;
        }   
    }
    return rc;
}

