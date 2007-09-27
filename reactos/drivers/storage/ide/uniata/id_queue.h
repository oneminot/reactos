#ifndef __UNIATA_COMMAND_QUEUE_SUPPORT__H__
#define __UNIATA_COMMAND_QUEUE_SUPPORT__H__

/*
    Insert command to proper place of command queue
    Perform reorder if necessary
 */
VOID
UniataQueueRequest(
    IN PHW_CHANNEL chan,
    IN PSCSI_REQUEST_BLOCK Srb
    );

/*
    Remove request from queue and get next request
 */
VOID
UniataRemoveRequest(
    IN PHW_CHANNEL chan,
    IN PSCSI_REQUEST_BLOCK Srb
    );

/*
    Get currently processed request
    (from head of the queue)
 */
PSCSI_REQUEST_BLOCK
UniataGetCurRequest(
    IN PHW_CHANNEL chan
    );

/*
    Get next channel to be serviced
    (used in simplex mode only)
 */
PHW_CHANNEL
UniataGetNextChannel(
    IN PHW_CHANNEL chan
    );

#endif __UNIATA_COMMAND_QUEUE_SUPPORT__H__