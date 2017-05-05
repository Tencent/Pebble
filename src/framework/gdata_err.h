#ifndef _PEBBLE_EXTENSION_GDATA_ERR_H_
#define _PEBBLE_EXTENSION_GDATA_ERR_H_


namespace pebble {
namespace oss {

#define DATA_LOG_SUCCESS 0

enum DATALOGERROR
{
    DATA_API_ERR_START_ERRNO      = -20000,     /*错误代码起始标志*/
    DATA_API_ERR_CREATE_CTX_LOG   = -20001,     /*初始化日志文件失败*/
    DATA_API_ERR_CREATE_CAT_LOG   = -2002,     /*获取日志类别失败*/
    DATA_API_ERR_CREATE_FILE_PATH = -20003,     /*创建日志保存路径失败*/
    DATA_API_ERR_END_ERRNO        = -20004,     /*错误代码结束标志*/
};


class CDataLogErr
{
public:
    /**
     * 根据错误代码获取错误信息
     * @param[in] err_code 错误代码
     *
     * @return  错误信息串的指针
     */
    static const char* DataLogErrMsg(int err_code) {
        switch (err_code) {
            case 0:
                return "no error";
            case DATA_API_ERR_CREATE_CTX_LOG:
                return "create log ctx failed";
            case DATA_API_ERR_CREATE_CAT_LOG:
                return "create log cat failed";
            case DATA_API_ERR_CREATE_FILE_PATH:
                return "create log file path failed";
            default:
                return "unknown error";
        }
    }
};
}// namespace oss
}// namespace pebble
#endif // _PEBBLE_EXTENSION_GDATA_ERR_H_

