.. _crc:

crc: cyclic redundancy checks
=============================


API
---

.. c:macro:: DMR_CRC8_MASK_VOICE_PI
.. c:macro:: DMR_CRC8_MASK_VOICE_LC
.. c:macro:: DMR_CRC8_MASK_TERMINATOR_WITH_LC

.. c:function:: void dmr_crc9(uint16_t *, uint8_t, uint8_t)

.. c:function:: void dmr_crc9_finish(uint16_t *, uint8_t)

.. c:function:: void dmr_crc16(uint16_t *, uint8_t)

.. c:function:: void dmr_crc16_finish(uint16_t *)

.. c:function:: void dmr_crc32(uint32_t *, uint8_t)

.. c:function:: void dmr_crc32_finish(uint32_t *)
