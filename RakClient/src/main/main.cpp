#include <nan.h>
#include <uv.h>
#include "log.h"
#include "main.h"

RakClientInterface *pRakClient = NULL;
bool IsConnectionRequired = false;

int iAreWeConnected = 0, iConnectionRequested = 0, iSpawned = 0, iGameInited = 0, iSpawnsAvailable = 0;
int iReconnectTime = 2 * 1000, iNotificationDisplayedBeforeSpawn = 0;
PLAYERID g_myPlayerID;
char g_szNickName[32];
struct stPlayerInfo playerInfo[MAX_PLAYERS];
struct stVehiclePool vehiclePool[MAX_VEHICLES];

// Persistent reference to JS emit callback
static Nan::Persistent<v8::Function> g_emitCallback;
static uv_timer_t g_networkTimer;

// ------------------------------------------------------------------
// EmitEvent: call g_emitCallback("eventName", dataObject) from C++
// Safe to call from RPC handlers (they run on the libuv event loop
// because the network timer ticks on the main JS thread).
// ------------------------------------------------------------------
void EmitEvent(const char* name, v8::Local<v8::Object> data) {
    if (g_emitCallback.IsEmpty()) return;
    Nan::HandleScope scope;
    v8::Local<v8::Function> emit = Nan::New(g_emitCallback);
    v8::Local<v8::Value> args[2] = {
        Nan::New(name).ToLocalChecked(),
        data
    };
    emit->Call(Nan::GetCurrentContext(), Nan::GetCurrentContext()->Global(), 2, args);
}

// Legacy helper kept for ClientMessage (strips colour codes already)
void MessageReceived(char* message) {
    Nan::HandleScope scope;
    v8::Local<v8::Object> data = Nan::New<v8::Object>();
    Nan::Set(data, Nan::New("text").ToLocalChecked(), Nan::New(message).ToLocalChecked());
    EmitEvent("clientMessage", data);
}

// ------------------------------------------------------------------
// Network timer — ticks every 10 ms on the libuv event loop.
// Keeps UpdateNetwork() off the main JS thread but on the same loop,
// so RPC handlers can safely call EmitEvent / JS functions directly.
// ------------------------------------------------------------------
static void NetworkTick(uv_timer_t* handle) {
    if (!pRakClient) return;
    UpdateNetwork(pRakClient);
    if (IsConnectionRequired) {
        Log("Connecting to %s", settings.server.szAddr);
        int id = (int)pRakClient->Connect(settings.server.szAddr, settings.server.iPort, 0, 0, 5);
        Log(id > 0 ? "Connected" : "Connection failed");
        IsConnectionRequired = false;
    }
    // Send periodic onfoot sync data (like the real client does ~30fps)
    if (iGameInited && iSpawned) {
        onFootUpdateAtNormalPos();
    }
}

// ------------------------------------------------------------------
// connect(emitCallback)
// Exported to JS. Loads XML config, sets up the network timer.
// ------------------------------------------------------------------
NAN_METHOD(Connect) {
    if (info.Length() < 1 || !info[0]->IsFunction()) {
        Nan::ThrowTypeError("connect(emitCallback) — first argument must be a function");
        return;
    }

    g_emitCallback.Reset(info[0].As<v8::Function>());

    int setLoaded = LoadSettings();
    Log("Settings Loaded %i", setLoaded);
    strcpy(g_szNickName, settings.server.szNickname);

    srand((unsigned int)GetTickCount());
    pRakClient = RakNetworkFactory::GetRakClientInterface();
    if (!pRakClient) {
        Nan::ThrowError("Failed to create RakClient interface");
        return;
    }
    pRakClient->SetMTUSize(576);
    RegisterRPCs(pRakClient);

    uv_timer_init(uv_default_loop(), &g_networkTimer);
    uv_timer_start(&g_networkTimer, NetworkTick, 0, 10);

    IsConnectionRequired = true;
}

// ------------------------------------------------------------------
// disconnect()
// ------------------------------------------------------------------
NAN_METHOD(Disconnect) {
    uv_timer_stop(&g_networkTimer);
    if (pRakClient) {
        pRakClient->Disconnect(300);
        RakNetworkFactory::DestroyRakClientInterface(pRakClient);
        pRakClient = NULL;
    }
    iAreWeConnected = 0;
    iGameInited = 0;
    iSpawned = 0;
}

// ------------------------------------------------------------------
// sendCommand(cmdString)
// ------------------------------------------------------------------
NAN_METHOD(SendCommandJS) {
    if (info.Length() < 1 || !info[0]->IsString()) {
        Nan::ThrowTypeError("sendCommand(cmd) — first argument must be a string");
        return;
    }
    v8::String::Utf8Value cmd(info.GetIsolate(), info[0]);
    sendServerCommand(*cmd);
}

// ------------------------------------------------------------------
// sendChat(text)
// ------------------------------------------------------------------
NAN_METHOD(SendChatJS) {
    if (info.Length() < 1 || !info[0]->IsString()) {
        Nan::ThrowTypeError("sendChat(text) — first argument must be a string");
        return;
    }
    v8::String::Utf8Value text(info.GetIsolate(), info[0]);
    sendChat(*text);
}

// ------------------------------------------------------------------
// respondDialog(dialogId, buttonId, listItem, inputText)
// ------------------------------------------------------------------
NAN_METHOD(RespondDialogJS) {
    if (info.Length() < 4) {
        Nan::ThrowTypeError("respondDialog(dialogId, buttonId, listItem, inputText)");
        return;
    }
    int dialogId  = Nan::To<int>(info[0]).FromJust();
    int buttonId  = Nan::To<int>(info[1]).FromJust();
    int listItem  = Nan::To<int>(info[2]).FromJust();
    v8::String::Utf8Value inputText(info.GetIsolate(), info[3]);
    sendDialogResponse(dialogId, buttonId, listItem, *inputText);
}

// ------------------------------------------------------------------
// spawn()
// ------------------------------------------------------------------
NAN_METHOD(SpawnJS) {
    sampSpawn();
}

// ------------------------------------------------------------------
// requestClass(classId)
// ------------------------------------------------------------------
NAN_METHOD(RequestClassJS) {
    int classId = info.Length() > 0 ? Nan::To<int>(info[0]).FromJust() : 0;
    sampRequestClass(classId);
}

// ------------------------------------------------------------------
// setPosition(x, y, z, angle)
// Updates the bot's reported position. Sync data is sent automatically
// by the NetworkTick timer via onFootUpdateAtNormalPos().
// ------------------------------------------------------------------
NAN_METHOD(SetPositionJS) {
    if (info.Length() < 3) {
        Nan::ThrowTypeError("setPosition(x, y, z [, angle])");
        return;
    }
    settings.fNormalModePos[0] = (float)Nan::To<double>(info[0]).FromJust();
    settings.fNormalModePos[1] = (float)Nan::To<double>(info[1]).FromJust();
    settings.fNormalModePos[2] = (float)Nan::To<double>(info[2]).FromJust();
    if (info.Length() > 3) {
        settings.fNormalModeRot = (float)Nan::To<double>(info[3]).FromJust();
    }
}

// ------------------------------------------------------------------
// Module init
// ------------------------------------------------------------------
NAN_MODULE_INIT(Init) {
    Nan::Set(target, Nan::New("connect").ToLocalChecked(),
             Nan::GetFunction(Nan::New<v8::FunctionTemplate>(Connect)).ToLocalChecked());
    Nan::Set(target, Nan::New("disconnect").ToLocalChecked(),
             Nan::GetFunction(Nan::New<v8::FunctionTemplate>(Disconnect)).ToLocalChecked());
    Nan::Set(target, Nan::New("sendCommand").ToLocalChecked(),
             Nan::GetFunction(Nan::New<v8::FunctionTemplate>(SendCommandJS)).ToLocalChecked());
    Nan::Set(target, Nan::New("sendChat").ToLocalChecked(),
             Nan::GetFunction(Nan::New<v8::FunctionTemplate>(SendChatJS)).ToLocalChecked());
    Nan::Set(target, Nan::New("respondDialog").ToLocalChecked(),
             Nan::GetFunction(Nan::New<v8::FunctionTemplate>(RespondDialogJS)).ToLocalChecked());
    Nan::Set(target, Nan::New("spawn").ToLocalChecked(),
             Nan::GetFunction(Nan::New<v8::FunctionTemplate>(SpawnJS)).ToLocalChecked());
    Nan::Set(target, Nan::New("requestClass").ToLocalChecked(),
             Nan::GetFunction(Nan::New<v8::FunctionTemplate>(RequestClassJS)).ToLocalChecked());
    Nan::Set(target, Nan::New("setPosition").ToLocalChecked(),
             Nan::GetFunction(Nan::New<v8::FunctionTemplate>(SetPositionJS)).ToLocalChecked());
}

NODE_MODULE(NODE_GYP_MODULE_NAME, Init);
