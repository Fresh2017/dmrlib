.. _log::

log: logging
============

Data types
----------

.. c:type:: dmr_log_priority_t
.. c:var:: DMR_LOG_PRIORITY_TRACE
.. c:var:: DMR_LOG_PRIORITY_DEBUG
.. c:var:: DMR_LOG_PRIORITY_INFO
.. c:var:: DMR_LOG_PRIORITY_WARN
.. c:var:: DMR_LOG_PRIORITY_ERROR
.. c:var:: DMR_LOG_PRIORITY_CRITICAL
.. c:var:: DMR_LOG_PRIORITIES

.. c:type:: void (*dmr_log_cb_t)(void *, dmr_log_priority_t, const char *)

API
---

.. c:macro:: DMR_LOG_MESSAGE_MAX
.. c:macro:: DMR_LOG_TIME_FORMAT
.. c:macro:: DMR_LOG_BOOL(x)

.. c:function:: bool dmr_log_color(void)
.. c:function:: void dmr_log_color_set(bool)
.. c:function:: dmr_log_priority_t dmr_log_priority(void)
.. c:function:: void dmr_log_priority_set(dmr_log_priority_t)
.. c:function:: void dmr_log_priority_reset(void)
.. c:function:: const char *dmr_log_prefix(void)
.. c:function:: void dmr_log_prefix_set(const char *)
.. c:function:: void dmr_log(const char *, ...)
.. c:function:: void dmr_log_mutex(const char *, ...)
.. c:function:: void dmr_log_trace(const char *, ...)
.. c:function:: void dmr_log_debug(const char *, ...)
.. c:function:: void dmr_log_info(const char *, ...)
.. c:function:: void dmr_log_warn(const char *, ...)
.. c:function:: void dmr_log_error(const char *, ...)
.. c:function:: void dmr_log_errno(const char *msg)
.. c:function:: void dmr_log_critical(const char *, ...)
.. c:function:: void dmr_log_message(dmr_log_priority_t, const char *, ...)
.. c:function:: void dmr_log_messagev(dmr_log_priority_t, const char *, va_list)
.. c:function:: void dmr_log_cb_get(dmr_log_cb_t *, void **)
.. c:function:: void dmr_log_cb(dmr_log_cb_t, void *)
