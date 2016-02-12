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

-- defined in dmr/protocol.h
PROTOCOL_UNKNOWN  = 0x00
PROTOCOL_HOMEBREW = 0x01
PROTOCOL_MMDVM    = 0x40
PROTOCOL_MBE      = 0xf0
PROTOCOL_SMS      = 0xf1

-- helper functions
local protocols   = {
    {protocol = PROTOCOL_UNKNOWN,  name = "unknown"},
    {protocol = PROTOCOL_HOMEBREW, name = "homebrew"},
    {protocol = PROTOCOL_MMDVM,    name = "mmdvm"},
    {protocol = PROTOCOL_MBE,      name = "mbe"},
    {protocol = PROTOCOL_REPEATER, name = "repeater"},
    {protocol = PROTOCOL_SMS,      name = "sms"},
}

function protocol_name(protocol_type)
    for i = 1, #protocols do
        if protocols[i].protocol == protocol_type then
            return protocols[i].name
        end
    end
    return "?"
end

function protocol_config(protocol)
    for i = 1, #config do
        -- log.debug("dmrlib.lua: iterate " .. tostring(i) .. ": " .. config[i].name)
        if config[i].name == protocol.name then
            return config[i]
        end
    end
    log.error("dmrlib.lua: no config found for protocol " .. protocol.name)
    return nil
end
