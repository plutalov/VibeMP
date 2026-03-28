/*
 *  RpcLogger — OMP component that logs all incoming/outgoing RPCs and packets.
 *
 *  Drop RpcLogger.dll into Server/components/ and restart.
 *  All RPC traffic appears in the server console and log.txt.
 */

#include <sdk.hpp>
#include <bitstream.hpp>

class RpcLoggerComponent final
    : public IComponent
    , public NetworkInEventHandler
    , public NetworkOutEventHandler
{
private:
    ICore* core = nullptr;

public:
    PROVIDE_UID(0xD3BF7A0E42C19501)

    // ── IComponent ──────────────────────────────────────────────

    StringView componentName() const override
    {
        return "RpcLogger";
    }

    SemanticVersion componentVersion() const override
    {
        return SemanticVersion(0, 1, 0, 0);
    }

    void onLoad(ICore* c) override
    {
        core = c;

        for (INetwork* network : core->getNetworks())
        {
            network->getInEventDispatcher().addEventHandler(this);
            network->getOutEventDispatcher().addEventHandler(this);
        }

        core->logLn(LogLevel::Message, "[RpcLogger] Loaded — logging all RPCs and packets.");
    }

    void onInit(IComponentList* components) override {}
    void onReady() override {}

    void free() override
    {
        if (core)
        {
            for (INetwork* network : core->getNetworks())
            {
                network->getInEventDispatcher().removeEventHandler(this);
                network->getOutEventDispatcher().removeEventHandler(this);
            }
        }
        delete this;
    }

    void reset() override {}

    // ── NetworkInEventHandler ───────────────────────────────────

    bool onReceiveRPC(IPlayer& peer, int id, NetworkBitStream& bs) override
    {
        core->logLn(LogLevel::Message,
            "[RPC-RECV] player=%d  id=%d  bits=%d",
            peer.getID(), id, (int)bs.GetNumberOfUnreadBits());
        return true;
    }

    bool onReceivePacket(IPlayer& peer, int id, NetworkBitStream& bs) override
    {
        core->logLn(LogLevel::Message,
            "[PKT-RECV] player=%d  id=%d  bits=%d",
            peer.getID(), id, (int)bs.GetNumberOfUnreadBits());
        return true;
    }

    // ── NetworkOutEventHandler ──────────────────────────────────

    bool onSendRPC(IPlayer* peer, int id, NetworkBitStream& bs) override
    {
        core->logLn(LogLevel::Message,
            "[RPC-SEND] player=%d  id=%d  bits=%d",
            peer ? peer->getID() : -1, id, (int)bs.GetNumberOfBitsUsed());
        return true;
    }

    bool onSendPacket(IPlayer* peer, int id, NetworkBitStream& bs) override
    {
        core->logLn(LogLevel::Message,
            "[PKT-SEND] player=%d  id=%d  bits=%d",
            peer ? peer->getID() : -1, id, (int)bs.GetNumberOfBitsUsed());
        return true;
    }
};

COMPONENT_ENTRY_POINT()
{
    return new RpcLoggerComponent();
}
