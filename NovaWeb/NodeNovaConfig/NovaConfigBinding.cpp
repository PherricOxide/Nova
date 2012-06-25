#define BUILDING_NODE_EXTENSION
#include <node.h>
#include "NovaConfigBinding.h"
#include "v8Helper.h"

using namespace v8;
using namespace Nova;
using namespace std;

NovaConfigBinding::NovaConfigBinding() {};
NovaConfigBinding::~NovaConfigBinding() {};

Config * NovaConfigBinding::GetChild()
{
  return m_conf;
}

void NovaConfigBinding::Init(Handle<Object> target) {
  // Prepare constructor template
  Local<FunctionTemplate> tpl = FunctionTemplate::New(New);
  tpl->SetClassName(String::NewSymbol("NovaConfigBinding"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);
  // Prototype
  tpl->PrototypeTemplate()->Set(String::NewSymbol("ListInterfaces"),FunctionTemplate::New(InvokeWrappedMethod<std::vector<std::string>, NovaConfigBinding, Config, &Config::ListInterfaces>));
  tpl->PrototypeTemplate()->Set(String::NewSymbol("ListLoopbacks"),FunctionTemplate::New(InvokeWrappedMethod<std::vector<std::string>, NovaConfigBinding, Config, &Config::ListLoopbacks>));
  tpl->PrototypeTemplate()->Set(String::NewSymbol("GetInterfaces"),FunctionTemplate::New(InvokeWrappedMethod<std::vector<std::string>, NovaConfigBinding, Config, &Config::GetInterfaces>));
  tpl->PrototypeTemplate()->Set(String::NewSymbol("GetUseAllInterfacesBinding"),FunctionTemplate::New(InvokeWrappedMethod<std::string, NovaConfigBinding, Config, &Config::GetUseAllInterfacesBinding>));
  tpl->PrototypeTemplate()->Set(String::NewSymbol("SetIfaces"),FunctionTemplate::New(SetIfaces)->GetFunction());
  tpl->PrototypeTemplate()->Set(String::NewSymbol("UseAllInterfaces"),FunctionTemplate::New(UseAllInterfaces)->GetFunction());
  tpl->PrototypeTemplate()->Set(String::NewSymbol("GetUseAnyLoopback"),FunctionTemplate::New(InvokeWrappedMethod<bool, NovaConfigBinding, Config, &Config::GetUseAnyLoopback>));
  tpl->PrototypeTemplate()->Set(String::NewSymbol("UseAnyLoopback"),FunctionTemplate::New(UseAnyLoopback)->GetFunction());
  tpl->PrototypeTemplate()->Set(String::NewSymbol("ReadSetting"),FunctionTemplate::New(ReadSetting)->GetFunction());
  tpl->PrototypeTemplate()->Set(String::NewSymbol("WriteSetting"),FunctionTemplate::New(WriteSetting)->GetFunction());

  Persistent<Function> constructor = Persistent<Function>::New(tpl->GetFunction());
  target->Set(String::NewSymbol("NovaConfigBinding"), constructor);
}

Handle<Value> NovaConfigBinding::New(const Arguments& args) {
  HandleScope scope;

  NovaConfigBinding* obj = new NovaConfigBinding();
  obj->m_conf = Config::Inst();
  obj->Wrap(args.This());

  return args.This();
}

Handle<Value> NovaConfigBinding::SetIfaces(const Arguments& args)
{
  HandleScope scope;
  NovaConfigBinding* obj = ObjectWrap::Unwrap<NovaConfigBinding>(args.This());
  
  if(args.Length() < 1)
  {
      return ThrowException(Exception::TypeError(String::New("Must be invoked with one parameter")));
  }
  
  std::vector<std::string> pass = cvv8::CastFromJS<std::vector<std::string>>(args[0]);
  
  for(uint i = 0; i < pass.size(); i++)
  {
    std::cout << "pass[" << i << "] == " << pass[i] << std::endl;
  }
  
  obj->m_conf->SetInterfaces(pass);
  
  return args.This();
}

Handle<Value> NovaConfigBinding::UseAllInterfaces(const Arguments& args) 
{
  HandleScope scope;
  NovaConfigBinding* obj = ObjectWrap::Unwrap<NovaConfigBinding>(args.This());

    if( args.Length() < 1 )
    {
        return ThrowException(Exception::TypeError(String::New("Must be invoked with one parameter")));
    }

    std::string def = cvv8::CastFromJS<std::string>( args[0] );

  std::cout << def << " is the value of def" << std::endl;

  if(!def.compare("true"))
  {
    obj->m_conf->SetUseAllInterfaces(true);
  }
  else
  {
    obj->m_conf->SetUseAllInterfaces(false);
  }
  
  std::cout << obj->m_conf->GetUseAllInterfacesBinding() << " is the new value of m_ifIsDefault" << std::endl;

  return args.This();
}

Handle<Value> NovaConfigBinding::UseAnyLoopback(const Arguments& args) 
{
  HandleScope scope;
  NovaConfigBinding* obj = ObjectWrap::Unwrap<NovaConfigBinding>(args.This());

    if( args.Length() < 1 )
    {
        return ThrowException(Exception::TypeError(String::New("Must be invoked with one parameter")));
    }

    bool def = cvv8::CastFromJS<bool>( args[0] );

  obj->m_conf->SetUseAnyLoopback(def);

  return args.This();
}

Handle<Value> NovaConfigBinding::ReadSetting(const Arguments& args) 
{
	HandleScope scope;
	NovaConfigBinding* obj = ObjectWrap::Unwrap<NovaConfigBinding>(args.This());

    if( args.Length() < 1 )
    {
        return ThrowException(Exception::TypeError(String::New("Must be invoked with one parameter")));
    }

    std::string p1 = cvv8::CastFromJS<std::string>( args[0] );


	return scope.Close(String::New(obj->m_conf->ReadSetting(p1).c_str()));
}


Handle<Value> NovaConfigBinding::WriteSetting(const Arguments& args) 
{
	HandleScope scope;
	NovaConfigBinding* obj = ObjectWrap::Unwrap<NovaConfigBinding>(args.This());

    if( args.Length() != 2 )
    {
        return ThrowException(Exception::TypeError(String::New("Must be invoked with two parameters")));
    }

    std::string key = cvv8::CastFromJS<std::string>( args[0] );
    std::string value = cvv8::CastFromJS<std::string>( args[1] );


	return scope.Close(Boolean::New(obj->m_conf->WriteSetting(key, value)));
}

