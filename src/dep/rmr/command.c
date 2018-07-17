#include "command.h"
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

struct mrCommandConf {
  const char *command;
  MRCommandFlags flags;
  int keyPos;
  int partitionKeyPos;
};

struct mrCommandConf __commandConfig[] = {

    // document commands
    {"_FT.SEARCH", MRCommand_Read | MRCommand_SingleKey, 1, 1},
    {"_FT.DEL", MRCommand_Write | MRCommand_MultiKey, 1, 2},
    {"_FT.GET", MRCommand_Read | MRCommand_MultiKey, 1, 2},
    {"_FT.MGET", MRCommand_Read | MRCommand_MultiKey, 1, 2},

    {"_FT.ADD", MRCommand_Write | MRCommand_MultiKey, 1, 2},
    {"_FT.ADDHASH", MRCommand_Write | MRCommand_MultiKey, 1, 2},

    // index commands
    {"_FT.CREATE", MRCommand_Write | MRCommand_SingleKey, 1, 1},
    {"_FT.ALTER", MRCommand_Write | MRCommand_SingleKey, 1, 1},
    {"_FT.DROP", MRCommand_Write | MRCommand_SingleKey, 1, 1},
    {"_FT.OPTIMIZE", MRCommand_Write | MRCommand_SingleKey, 1, 1},
    {"_FT.INFO", MRCommand_Read | MRCommand_SingleKey, 1, 1},
    {"_FT.EXPLAIN", MRCommand_Read | MRCommand_SingleKey, 1, 1},
    {"_FT.TAGVALS", MRCommand_Read | MRCommand_SingleKey, 1, 1},

    // Suggest commands
    {"_FT.SUGADD", MRCommand_Write | MRCommand_SingleKey, 1, 1},
    {"_FT.SUGGET", MRCommand_Read | MRCommand_SingleKey, 1, 1},
    {"_FT.SUGLEN", MRCommand_Read | MRCommand_SingleKey, 1, 1},
    {"_FT.SUGDEL", MRCommand_Write | MRCommand_SingleKey, 1, 1},
    {"_FT.CURSOR", MRCommand_Read | MRCommand_SingleKey, 2, 2},

    // Synonyms commands
    {"_FT.SYNADD", MRCommand_Write | MRCommand_NoKey, 1, -1},
    {"_FT.SYNDUMP", MRCommand_Write | MRCommand_NoKey, 1, -1},
    {"_FT.SYNUPDATE", MRCommand_Write | MRCommand_NoKey, 1, -1},
    {"_FT.SYNFORCEUPDATE", MRCommand_Write | MRCommand_NoKey, 1, -1},

    // Coordination commands - they are all read commands since they can be triggered from slaves
    {"FT.ADD", MRCommand_Read | MRCommand_Coordination, -1, 2},
    {"FT.SEARCH", MRCommand_Read | MRCommand_Coordination, -1, 1},
    {"FT.AGGREGATE", MRCommand_Read | MRCommand_Coordination, -1, 1},

    {"FT.EXPLAIN", MRCommand_Read | MRCommand_Coordination, -1, 1},

    {"FT.FSEARCH", MRCommand_Read | MRCommand_Coordination, -1, 1},
    {"FT.CREATE", MRCommand_Read | MRCommand_Coordination, -1, 1},
    {"FT.CLUSTERINFO", MRCommand_Read | MRCommand_Coordination, -1, -1},
    {"FT.INFO", MRCommand_Read | MRCommand_Coordination, -1, 1},
    {"FT.ADDHASH", MRCommand_Read | MRCommand_Coordination, -1, 2},
    {"FT.DEL", MRCommand_Read | MRCommand_Coordination, -1, 2},
    {"FT.DROP", MRCommand_Read | MRCommand_Coordination, -1, 1},
    {"FT.CREATE", MRCommand_Read | MRCommand_Coordination, -1, 1},
    {"FT.GET", MRCommand_Read | MRCommand_Coordination, -1, 2},
    {"FT.MGET", MRCommand_Read | MRCommand_Coordination, -1, 2},

    // Auto complete coordination commands
    {"FT.SUGADD", MRCommand_Read | MRCommand_Coordination, -1, 1},
    {"FT.SUGGET", MRCommand_Read | MRCommand_Coordination, -1, 1},
    {"FT.SUGDEL", MRCommand_Read | MRCommand_Coordination, -1, 1},
    {"FT.SUGLEN", MRCommand_Read | MRCommand_Coordination, -1, 1},

    {"KEYS", MRCommand_Read | MRCommand_NoKey, -1, -1},
    {"INFO", MRCommand_Read | MRCommand_NoKey, -1, -1},
    {"SCAN", MRCommand_Read | MRCommand_NoKey, -1, -1},

    // sentinel
    {NULL},
};

int _getCommandConfId(MRCommand *cmd) {
  cmd->id = -1;
  if (cmd->num == 0) {
    return 0;
  }

  for (int i = 0; __commandConfig[i].command != NULL; i++) {
    if (!strcasecmp(cmd->strs[0], __commandConfig[i].command)) {
      // printf("conf id for cmd %s: %d\n", cmd->strs[0], i);
      cmd->id = i;
      return 1;
    }
  }
  return 0;
}

void MRCommand_Free(MRCommand *cmd) {
  for (int i = 0; i < cmd->num; i++) {
    free(cmd->strs[i]);
  }
  free(cmd->strs);
  free(cmd->lens);
}

static void assignStr(MRCommand *cmd, size_t idx, const char *s, size_t n) {
  char *news = malloc(n + 1);
  cmd->strs[idx] = news;
  cmd->lens[idx] = n;
  news[n] = 0;
  memcpy(news, s, n);
}

static void assignCstr(MRCommand *cmd, size_t idx, const char *s) {
  assignStr(cmd, idx, s, strlen(s));
}

static void copyStr(MRCommand *dst, size_t dstidx, const MRCommand *src, size_t srcidx) {
  const char *srcs = src->strs[srcidx];
  size_t srclen = src->lens[srcidx];

  assignStr(dst, dstidx, srcs, srclen);
}

static void assignRstr(MRCommand *dst, size_t idx, RedisModuleString *src) {
  size_t n;
  const char *s = RedisModule_StringPtrLen(src, &n);
  assignStr(dst, idx, s, n);
}

static void MRCommand_Init(MRCommand *cmd, size_t len) {
  cmd->num = len;
  cmd->strs = malloc(sizeof(*cmd->strs) * len);
  cmd->lens = malloc(sizeof(*cmd->lens) * len);
  cmd->id = 0;
}

MRCommand MR_NewCommandArgv(int argc, char **argv) {
  MRCommand cmd = {.num = argc};
  MRCommand_Init(&cmd, argc);

  for (int i = 0; i < argc; i++) {
    assignCstr(&cmd, i, argv[i]);
  }
  _getCommandConfId(&cmd);
  return cmd;
}

/* Create a deep copy of a command by duplicating all strings */
MRCommand MRCommand_Copy(const MRCommand *cmd) {
  MRCommand ret;
  MRCommand_Init(&ret, cmd->num);
  ret.id = cmd->id;

  for (int i = 0; i < cmd->num; i++) {
    copyStr(&ret, i, cmd, i);
  }
  return ret;
}

MRCommand MR_NewCommand(int argc, ...) {
  MRCommand cmd;
  MRCommand_Init(&cmd, argc);

  va_list ap;
  va_start(ap, argc);
  for (int i = 0; i < argc; i++) {
    assignCstr(&cmd, i, va_arg(ap, const char *));
  }
  va_end(ap);
  _getCommandConfId(&cmd);
  return cmd;
}

MRCommand MR_NewCommandFromStrings(int argc, char **argv) {
  MRCommand cmd;
  MRCommand_Init(&cmd, argc);
  for (size_t ii = 0; ii < argc; ++ii) {
    assignCstr(&cmd, ii, argv[ii]);
  }
  _getCommandConfId(&cmd);
  return cmd;
}

MRCommand MR_NewCommandFromRedisStrings(int argc, RedisModuleString **argv) {
  MRCommand cmd;
  MRCommand_Init(&cmd, argc);
  for (int i = 0; i < argc; i++) {
    assignRstr(&cmd, i, argv[i]);
  }
  _getCommandConfId(&cmd);
  return cmd;
}

static void extendCommandList(MRCommand *cmd, size_t toAdd) {
  cmd->num = toAdd;
  cmd->strs = realloc(cmd->strs, sizeof(*cmd->strs) * cmd->num);
  cmd->lens = realloc(cmd->lens, sizeof(*cmd->lens) * cmd->num);
}

void MRCommand_AppendStringsArgs(MRCommand *cmd, int num, char **args) {
  if (num <= 0) return;
  int oldNum = cmd->num;
  extendCommandList(cmd, num);

  for (int i = oldNum; i < cmd->num; i++) {
    assignCstr(cmd, i, args[i]);
  }
}

void MRCommand_AppendArgs(MRCommand *cmd, int num, ...) {
  if (num <= 0) return;
  int oldNum = cmd->num;
  extendCommandList(cmd, num);

  va_list(ap);
  va_start(ap, num);
  for (int i = oldNum; i < cmd->num; i++) {
    assignCstr(cmd, i, va_arg(ap, const char *));
  }
  va_end(ap);
}

void MRCommand_AppendFrom(MRCommand *cmd, const MRCommand *srcCmd, size_t srcidx) {
  MRCommand_Append(cmd, srcCmd->strs[srcidx], srcCmd->lens[srcidx]);
}

void MRCommand_Append(MRCommand *cmd, const char *s, size_t n) {
  extendCommandList(cmd, cmd->num + 1);
  assignStr(cmd, cmd->num - 1, s, n);
  if (cmd->num == 1) {
    _getCommandConfId(cmd);
  }
}

/** Set the prefix of the command (i.e {prefix}.{command}) to a given prefix. If the command has a
 * module style prefx it gets replaced with the new prefix. If it doesn't, we prepend the prefix to
 * the command. */
void MRCommand_SetPrefix(MRCommand *cmd, const char *newPrefix) {

  char *suffix = strchr(cmd->strs[0], '.');
  if (!suffix) {
    suffix = cmd->strs[0];
  } else {
    suffix++;
  }

  char *buf = NULL;
  asprintf(&buf, "%s.%s", newPrefix, suffix);
  MRCommand_ReplaceArgNoDup(cmd, 0, buf, strlen(buf));
  _getCommandConfId(cmd);
}

void MRCommand_ReplaceArgNoDup(MRCommand *cmd, int index, const char *newArg, size_t len) {
  if (index < 0 || index >= cmd->num) {
    return;
  }
  char *tmp = cmd->strs[index];
  cmd->strs[index] = (char *)newArg;
  cmd->lens[index] = len;
  free(tmp);

  // if we've replaced the first argument, we need to reconfigure the command
  if (index == 0) {
    _getCommandConfId(cmd);
  }
}
void MRCommand_ReplaceArg(MRCommand *cmd, int index, const char *newArg, size_t len) {
  char *news = malloc(len + 1);
  news[len] = 0;
  memcpy(news, newArg, len);
  MRCommand_ReplaceArgNoDup(cmd, index, news, len);
}

MRCommandFlags MRCommand_GetFlags(MRCommand *cmd) {
  if (cmd->id < 0) return 0;
  return __commandConfig[cmd->id].flags;
}

int MRCommand_GetShardingKey(MRCommand *cmd) {
  if (cmd->id < 0) {
    return 1;  // default
  }

  return __commandConfig[cmd->id].keyPos;
}

int MRCommand_GetPartitioningKey(MRCommand *cmd) {
  if (cmd->id < 0) {
    return 1;  // default
  }

  return __commandConfig[cmd->id].partitionKeyPos;
}

/* Return 1 if the command should not be sharded (i.e a coordination command or system command) */
int MRCommand_IsUnsharded(MRCommand *cmd) {
  if (cmd->id < 0) {
    return 0;  // default
  }
  return __commandConfig[cmd->id].keyPos <= 0;
}

void MRCommand_Print(MRCommand *cmd) {
  MRCommand_FPrint(stdout, cmd);
}

void MRCommand_FPrint(FILE *fd, MRCommand *cmd) {
  for (int i = 0; i < cmd->num; i++) {
    fprintf(fd, "%.*s ", (int)cmd->lens[i], cmd->strs[i]);
  }
  fprintf(fd, "\n");
}