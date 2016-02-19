.. _bits:

bits: bit and byte manipulation
===============================


Data types
----------

.. c:type:: dmr_bit

   Single bit values.

.. c:type:: dmr_dibit

   Dibit (or double bit) values.

.. c:type:: dmr_tribit

   Tribit (or triple bit) values.

API
---

Bit sizes
^^^^^^^^^

.. c:macro:: DMR_SLOT_TYPE_HALF
.. c:macro:: DMR_SLOT_TYPE_BITS

Byte manipulation
^^^^^^^^^^^^^^^^^

.. c:macro:: DMR_UINT16_LE(b0, b1)
.. c:macro:: DMR_UINT16_BE(b0, b1)
.. c:macro:: DMR_UINT16_LE_PACK(b, n)
.. c:macro:: DMR_UINT16_BE_PACK(b, n)
.. c:macro:: DMR_UINT32_LE(b0, b1, b2, b3)
.. c:macro:: DMR_UINT32_BE(b0, b1, b2, b3)
.. c:macro:: DMR_UINT32_BE_UNPACK(b)
.. c:macro:: DMR_UINT32_BE_UNPACK3(b)
.. c:macro:: DMR_UINT32_LE_PACK(b, n)
.. c:macro:: DMR_UINT32_LE_PACK3(b, n)
.. c:macro:: DMR_UINT32_BE_PACK(b, n)
.. c:macro:: DMR_UINT32_BE_PACK3(b, n)

.. c:function:: char *dmr_byte_to_binary(uint8_t byte)
.. c:function:: uint8_t dmr_bits_to_byte(bool bits[8])
.. c:function:: void dmr_bits_to_bytes(bool *bits, size_t bits_length, uint8_t *bytes, size_t bytes_length)
.. c:function:: void dmr_byte_to_bits(uint8_t byte, bool bits[8])
.. c:function:: void dmr_bytes_to_bits(uint8_t *bytes, size_t bytes_length, bool *bits, size_t bits_length)
