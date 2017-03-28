
#ifndef __LIBRMR_H__
#define __LIBRMR_H__

#include "reply.h"
#include "cluster.h"
#include "command.h"

struct MRCtx;
struct RedisModuleCtx;

/* Prototype for all reduce functions */
typedef int (*MRReduceFunc)(struct MRCtx *ctx, int count, MRReply **replies);

/* Fanout map - send the same command to all the shards, sending the collective
 * reply to the reducer callback */
int MR_Fanout(struct MRCtx *ctx, MRReduceFunc reducer, MRCommand cmd);

int MR_Map(struct MRCtx *ctx, MRReduceFunc reducer, MRCommandGenerator cmds);

int MR_MapSingle(struct MRCtx *ctx, MRReduceFunc reducer, MRCommand cmd);

/* Initialize the MapReduce engine with a node provider */
void MR_Init(MRCluster *cl);

int MR_UpdateTopology(void *ctx);

/* Get the user stored private data from the context */
void *MRCtx_GetPrivdata(struct MRCtx *ctx);

struct RedisModuleCtx *MRCtx_GetRedisCtx(struct MRCtx *ctx);

/* Free the MapReduce context */
void MRCtx_Free(struct MRCtx *ctx);

/* Create a new MapReduce context with a given private data. In a redis module
 * this should be the RedisModuleCtx */
struct MRCtx *MR_CreateCtx(struct RedisModuleCtx *ctx, void *privdata);

#endif  //__LIBRMR_H__