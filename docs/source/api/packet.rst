.. _packet:

packet: DMR packet handling
===========================

Data types
----------

.. c:type:: dmr_color_code

.. c:type:: dmr_data_type

   DMR data type. Also includes pseudo data types for internal usage.

.. c:var:: DMR_DATA_TYPE_VOICE_PI
.. c:var:: DMR_DATA_TYPE_VOICE_LC
.. c:var:: DMR_DATA_TYPE_TERMINATOR_WITH_LC
.. c:var:: DMR_DATA_TYPE_CSBK
.. c:var:: DMR_DATA_TYPE_MBC_HEADER
.. c:var:: DMR_DATA_TYPE_MBC_CONTINUATION
.. c:var:: DMR_DATA_TYPE_DATA_HEADER
.. c:var:: DMR_DATA_TYPE_RATE12_DATA
.. c:var:: DMR_DATA_TYPE_RATE34_DATA
.. c:var:: DMR_DATA_TYPE_IDLE
.. c:var:: DMR_DATA_TYPE_INVALID
.. c:var:: DMR_DATA_TYPE_SYNC_VOICE
.. c:var:: DMR_DATA_TYPE_VOICE_SYNC
.. c:var:: DMR_DATA_TYPE_VOICE     

.. c:type:: dmr_id

   DMR identifier.

.. c:type:: dmr_fid

   DMR function identifier.

.. c:var:: DMR_FID_ETSI
.. c:var:: DMR_FID_DMRA
.. c:var:: DMR_FID_HYTERA
.. c:var:: DMR_FID_MOTOROLA

.. c:type:: dmr_flco

   Full Link Control Opcode.

.. c:var:: DMR_FLCO_GROUP
.. c:var:: DMR_FLCO_PRIVATE
.. c:var:: DMR_FLCO_INVALID

.. c:type:: dmr_ts

   DMR time slot.

.. c:var:: DMR_TS1
.. c:var:: DMR_TS2


Packets
^^^^^^^

.. c:type:: dmr_packet

   Raw DMR packet.

.. c:type:: dmr_packet_data

   Raw DMR packet data bits.

.. c:type:: dmr_packet_data_block_bits

   Raw DMR packet data block bits.

.. c:type:: dmr_packet_data_block

   Parsed DMR packet data block.

.. c:type:: int (*dmr_packet_cb)(dmr_packet, void *)

   Callback for raw DMR packets.

.. c:type:: dmr_parsed_packet

   Parsed DMR packet with meta data.

.. c:member:: dmr_packet     dmr_parsed_packet.packet
.. c:member:: dmr_ts         dmr_parsed_packet.ts
.. c:member:: dmr_flco       dmr_parsed_packet.flco
.. c:member:: dmr_id         dmr_parsed_packet.src_id

   Source (originating) DMR ID.

.. c:member:: dmr_id         dmr_parsed_packet.dst_id

   Target DMR ID.

.. c:member:: dmr_id         dmr_parsed_packet.repeater_id

   DMR ID of the repeater or hotspot.

.. c:member:: dmr_data_type  dmr_parsed_packet.data_type
.. c:member:: dmr_color_code dmr_parsed_packet.color_code
.. c:member:: uint8_t        dmr_parsed_packet.sequence

   Sequential number for frames that belong to the same
   :c:member:`dmr_parsed_packet.stream_id`.

.. c:member:: uint32_t       dmr_parsed_packet.stream_id

   Unique identifier for the stream.

.. c:member:: uint8_t        dmr_parsed_packet.voice_frame

   Voice frame index.

.. c:member:: bool           dmr_parsed_packet.parsed

.. c:type:: int (*dmr_parsed_packet_cb)(dmr_parsed_packet *, void *)

   Callback for parsed DMR packets.


API 
---

.. c:macro:: DMR_PACKET_LEN

   Size of a single DMR packet (frame) in bytes.

.. c:macro:: DMR_PACKET_BITS

   Size of a single DMR packet (frame) in bits.

.. c:macro:: DMR_DATA_TYPE_COUNT

   Number of valid data types, without pseudo data types.

.. note::

   The :c:func:`dmr_dump_packet` and :c:func:`dmr_dump_parsed_packet` functions
   are not ABI stable.

.. c:function:: void dmr_dump_packet(dmr_packet)

   Debug function that decodes a raw DMR packet.

.. c:function:: void dmr_dump_parsed_packet(dmr_parsed_packet *)

.. c:function:: dmr_parsed_packet *dmr_packet_decode(dmr_packet)

.. c:function:: char *dmr_flco_name(dmr_flco)

.. c:function:: char *dmr_ts_name(dmr_ts)

.. c:function:: char *dmr_fid_name(dmr_fid)

.. c:function:: char *dmr_data_type_name(dmr_data_type)

.. c:function:: char *dmr_data_type_name_short(dmr_data_type)

.. c:function:: int dmr_payload_bits(dmr_packet, void *)

.. c:function:: int dmr_slot_type_decode(dmr_packet, dmr_color_code *, dmr_data_type *)

.. c:function:: int dmr_slot_type_encode(dmr_packet, dmr_color_code, dmr_data_type)
