function route(src, dst, packet)
    if src == nil or dst == nil or packet == nil then
        log.error("route.lua: got a nil argument")
        -- Reject
        return false
    end

    log.debug("route.lua: "
        ..protocol_name(src.type).."("..src.name..")->"
        ..protocol_name(dst.type).."("..dst.name.."): "
        ..packet.src_id.."->"..packet.dst_id)

    -- Same protocol is denied
    if src.name == dst.name then
        -- Reject
        return false
    end

    -- All frames to the MMDVM modem
    if dst.type == PROTOCOL_MMDVM then
        local modified = false

        -- if packet.ts == TS1 then
        --     log.debug("route.lua: redirect to TS2")
        --     packet.ts = TS2
        --     modified = true
        -- end

        if packet.src_id == 0 then
            log.debug("route.lua: setting color code")
            packet.color_code = 1
            modified = true
        end

        if modified then
            log.debug("route.lua: modified packet to MMDVM")
            return packet
        end

        -- Permit unmodified
        log.debug("route.lua: permit unmodified to MMDVM")
        return true
    end

    -- Frames from MMDVM to Homebrew may need correcting
    if src.type == PROTOCOL_MMDVM and dst.type == PROTOCOL_HOMEBREW then
        -- Retrieve Homebrew config
        local cfg = protocol_config(dst)

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

io.write(protocol_name(PROTOCOL_HOMEBREW))
