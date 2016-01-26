-- defined in dmr/packet.h
TS1 = 0x00
TS2 = 0x01

FLCO_GROUP   = 0x00
FLCO_PRIVATE = 0x01

DATA_TYPE_VOICE_PI           = 0x00
DATA_TYPE_VOICE_LC           = 0x01
DATA_TYPE_TERMINATOR_WITH_LC = 0x02
DATA_TYPE_CSBK               = 0x03
DATA_TYPE_MBC_HEADER         = 0x04
DATA_TYPE_MBC_CONTINUATION   = 0x05
DATA_TYPE_DATA_HEADER        = 0x06
DATA_TYPE_RATE12_DATA        = 0x07
DATA_TYPE_RATE34_DATA        = 0x08
DATA_TYPE_IDLE               = 0x09
DATA_TYPE_INVALID            = 0x0a
DATA_TYPE_SYNC_VOICE         = 0x20
DATA_TYPE_VOICE_SYNC         = 0xf0
DATA_TYPE_VOICE              = 0xf1

-- defined in dmr/proto.h
PROTO_UNKNOWN  = 0x00
PROTO_HOMEBREW = 0x01
PROTO_MMDVM    = 0x10
PROTO_MBE      = 0x20
PROTO_REPEATER = 0xf0
PROTO_SMS      = 0xf1

-- helper functions
local protos   = {
    {proto = PROTO_UNKNOWN,  name = "unknown"},
    {proto = PROTO_HOMEBREW, name = "homebrew"},
    {proto = PROTO_MMDVM,    name = "mmdvm"},
    {proto = PROTO_MBE,      name = "mbe"},
    {proto = PROTO_REPEATER, name = "repeater"},
    {proto = PROTO_SMS,      name = "sms"},
}

function proto_name(proto_type)
    for i = 1, #protos do
        if protos[i].proto == proto_type then
            return protos[i].name
        end
    end
    return "?"
end

function proto_config(proto)
    for i = 1, #config do
        -- log.debug("dmrlib.lua: iterate " .. tostring(i) .. ": " .. config[i].name)
        if config[i].name == proto.name then
            return config[i]
        end
    end
    log.error("dmrlib.lua: no config found for proto " .. proto.name)
    return nil
end