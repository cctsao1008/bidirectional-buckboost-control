/**
 * @file host_service.h
 * @brief Background host-protocol service for USART1 communication.
 */

#ifndef BUCKBOOST_HOST_SERVICE_H
#define BUCKBOOST_HOST_SERVICE_H

/** Initialize host protocol state. */
void host_service_init(void);

/** Process all currently available UART receive bytes and dispatch valid frames. */
void host_service_run(void);

#endif /* BUCKBOOST_HOST_SERVICE_H */
