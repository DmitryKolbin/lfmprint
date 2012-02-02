#include "include/lfmprint.h"
#include <v8.h>
#include <node_version.h>
#include <node.h>
#include <iostream>
#include <string>

using namespace v8;
using namespace node;
using namespace std;

class lfmprintNode : ObjectWrap{
public:
    lfmprintNode() {
        lfmprint_init();
    }
    ~lfmprintNode(){
        lfmprint_destroy();
    }
    static Persistent<FunctionTemplate> persistent_function_template;
    static void init(Handle<Object> target) {
        HandleScope scope;
        Local<FunctionTemplate> t = FunctionTemplate::New(New);
        persistent_function_template = Persistent<FunctionTemplate>::New(t);
        persistent_function_template->InstanceTemplate()->SetInternalFieldCount(1);
        persistent_function_template->SetClassName(String::NewSymbol("lfmprint"));
        NODE_SET_PROTOTYPE_METHOD(persistent_function_template, "fetchFingerprint", fetchFingerprint);
        target->Set(String::NewSymbol("lfmprint"), persistent_function_template->GetFunction());
    }
    static Handle<Value> New(const Arguments& args) {
        HandleScope scope;
        lfmprintNode* lfm = new lfmprintNode();
        lfm->Wrap(args.This());
        return args.This();
    }
    struct lfmprint_thread_data_t {
        lfmprintNode * lfm;
        Persistent<Function> cb;
        string fp;
        string fname;
        string exception;
    };
    static Handle<Value> fetchFingerprint(const Arguments& args) {
        HandleScope scope;        
        if (args.Length() <= (1) || !args[0]->IsString() || !args[1]->IsFunction()) {
            return ThrowException(Exception::TypeError(String::New("Usage: <filename> <callback function>")));
        }

        lfmprintNode *lfm = ObjectWrap::Unwrap<lfmprintNode>(args.This());

        String::Utf8Value fname(args[0]);
        Local<Function> cb = Local<Function>::Cast(args[1]);

        lfmprint_thread_data_t *thread_data = new lfmprint_thread_data_t();
        
        if (!thread_data) {
            return ThrowException(Exception::TypeError(String::New("Couldn't initializate thread")));
        }
        
        thread_data->lfm = lfm;
        thread_data->cb = Persistent<Function>::New(cb);
        thread_data->fname = *fname;

        lfm->Ref();

        #if NODE_VERSION_AT_LEAST(0, 5, 4)
          eio_custom((void (*)(eio_req*))fetchFingerprintWorker, EIO_PRI_DEFAULT, AfterFetchFingerprint, thread_data);
        #else
          eio_custom(fetchFingerprintWorker, EIO_PRI_DEFAULT, AfterFetchFingerprint, thread_data);
        #endif
        ev_ref(EV_DEFAULT_UC);
        return Undefined();
    }

    static int fetchFingerprintWorker(eio_req *req) {
        lfmprint_thread_data_t *thread_data = static_cast<lfmprint_thread_data_t *> (req->data);
        try {
            thread_data->fp = getFingerprint(thread_data->fname);
        } catch (const char * err){
            thread_data->exception.append(err);
        }
        return 0;
    }
    static int AfterFetchFingerprint(eio_req * req) {
        HandleScope scope;
        lfmprint_thread_data_t *thread_data = static_cast<lfmprint_thread_data_t *> (req->data);
        ev_unref(EV_DEFAULT_UC);
        thread_data->lfm->Unref();
        Local<Value> argv[2];
        if (!thread_data->exception.empty()) {
            argv[0] = Local<String>::New(String::New(thread_data->exception.data(), thread_data->exception.size()));
            argv[1] = Local<Value>::New(Null());
        } else {
            argv[0] = Local<Value>::New(Null());
            argv[1] = Local<String>::New(String::New(thread_data->fp.data(), thread_data->fp.size()));
        }

        TryCatch try_catch;
        thread_data->cb->Call(Context::GetCurrent()->Global(), 2, argv);
        if (try_catch.HasCaught()) {
            FatalException(try_catch);
        }

        thread_data->cb.Dispose();
        delete thread_data;
        return 0;
    }
};

Persistent<FunctionTemplate> lfmprintNode::persistent_function_template;

extern "C" {
    static void init(Handle<Object> target) {
        lfmprintNode::init(target);
    }
    NODE_MODULE(lfmprint,init)
}
