function route(src, dst, packet)
    if src == nil or dst == nil or packet == nil then
        log.error("route.lua: got a nil argument")
        -- Reject
        return false
    end

    log.debug("route.lua: "
        ..proto_name(src.type).."("..src.name..")->"
        ..proto_name(dst.type).."("..dst.name..")")

    -- Same proto is denied
    if src.name == dst.name then
        -- Reject
        return false
    end

    -- All frames to the MMDVM modem should go unmodified
    if dst.type == PROTO_MMDVM then
        if packet.src_id == 0 then
            packet.color_code = 1
            packet.ts = TS2
            return packet
        end

        -- Permit unmodified
        return true
    end

    -- Frames from MMDVM to Homebrew may need correcting
    if src.type == PROTO_MMDVM and dst.type == PROTO_HOMEBREW then
        -- Retrieve Homebrew config
        local cfg = proto_config(dst)

        -- Always update repeater ID
        packet.repeater_id = cfg.repeater_id

        -- If there is no source set, use our repeater_id
        if packet.src_id == 0 then
            log.debug("route.lua: updating packet.src_id")
            packet.src_id = cfg.repeater_id
        end
        
        -- If there is no destination set, use the Parrot
        if packet.dst_id == 0 then
            log.debug("route.lua: updating packet.dst_id (and flco)")
            packet.dst_id = 9990
            packet.flco   = FLCO_PRIVATE
        end

        -- Permit with modified packet
        return packet
    end

    -- Permit unmodified
    return true
end

io.write(proto_name(PROTO_HOMEBREW))
