
#include "DispatchMsgService.h"
#include "interface.h"
#include "Logger.h"
#include "sqlconnection.h"
#include "BusProcessor.h"

#include <functional>


extern "C"  
{  
    #include "lua.h"  
    #include "lauxlib.h"  
    #include "lualib.h"  
}


int main(int argc, char** argv)
{
    if (argc != 2)
    {
        printf("please input brks <log file config>!\n");
        return -1;
    }
    
    if(!Logger::instance()->init(std::string(argv[1])))
    {
        printf("init log module failed.\n");
        return -1;
    }
    else
    {
        printf("init log module success!");
    }

	/*创建lua状态*/
	lua_State *L = luaL_newstate();
	if (L == NULL)  
    {
		printf("create lua_State error!");  
        return -1;  
    }

	/*加载lua文件*/
	int iRet = luaL_loadfile(L,"conf.lua");
	if( iRet )
	{
		printf("load lua file error!");
		return -1;
	}

	/*运行lua文件*/
	iRet = lua_pcall(L,0,0,0);
	if( iRet )
	{
		printf("pcall lua error!");
		return -1;
	}

	/*读取数据*/
	lua_getglobal(L,"mysqltab1");
	lua_getfield(L,-1,"dbname");
	lua_getfield(L,-2,"dbpasswd");
	lua_getfield(L,-3,"dbuser");
	lua_getfield(L,-4,"port");
	lua_getfield(L,-5,"host");

	std::string h = lua_tostring(L,-1);
	const char *host = h.c_str();
	int			port = lua_tonumber(L,-2);
	std::string user = lua_tostring(L,-3);
	const char *dbuser = user.c_str(); 
	std::string passwd = lua_tostring(L,-4);
	const char *dbpasswd = passwd.c_str();
	std::string name = lua_tostring(L,-5);
	const char *dbname = name.c_str();

	lua_getglobal(L,"intfPort");  
    int intfPort = lua_tonumber(L,-1); 
	LOG_INFO("host:[%s],port:[%d],user:[%s],passwd:[%s],dbname:[%s],intfPort:[%d]",host,port,dbuser,dbpasswd,dbname,intfPort);

    std::shared_ptr<DispatchMsgService> dms(new DispatchMsgService);
    dms->open();
    
    std::shared_ptr<MysqlConnection> mysqlconn(new MysqlConnection);
    mysqlconn->Init(host, port, dbuser, dbpasswd, dbname);
    
    BusinessProcessor processor(dms, mysqlconn);
    processor.init();

    std::function< iEvent* (const iEvent*)> fun = std::bind(&DispatchMsgService::process, dms.get(), std::placeholders::_1);
    
    Interface intf(fun);
    intf.start(intfPort);

    LOG_INFO("brks start successful!");

    for(;;);

	/*关闭state*/      
	lua_close(L);
    
    return 0;
}
