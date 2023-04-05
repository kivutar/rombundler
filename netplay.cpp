#include <stdlib.h>
#include <unistd.h>
#include <ggponet.h>
#include <platform_unix.h>
#include "nongamestate.h"

NonGameState ngs = { 0 };

int
fletcher32_checksum(short *data, size_t len)
{
   int sum1 = 0xffff, sum2 = 0xffff;

   while (len) {
      unsigned tlen = len > 360 ? 360 : len;
      len -= tlen;
      do {
         sum1 += *data++;
         sum2 += sum1;
      } while (--tlen);
      sum1 = (sum1 & 0xffff) + (sum1 >> 16);
      sum2 = (sum2 & 0xffff) + (sum2 >> 16);
   }

   /* Second reduction step to reduce sums to 16 bits */
   sum1 = (sum1 & 0xffff) + (sum1 >> 16);
   sum2 = (sum2 & 0xffff) + (sum2 >> 16);
   return sum2 << 16 | sum1;
}

bool __cdecl
net_begin_game_callback(const char *name)
{
   return true;
}

bool __cdecl
net_advance_frame_callback(int flags)
{
   // int inputs[MAX_SHIPS] = { 0 };
   // int disconnect_flags;

   // // Make sure we fetch new inputs from GGPO and use those to update
   // // the game state instead of reading from the keyboard.
   // ggpo_synchronize_input(ggpo, (void *)inputs, sizeof(int) * MAX_SHIPS, &disconnect_flags);
   // VectorWar_AdvanceFrame(inputs, disconnect_flags);
   return true;
}

bool __cdecl
net_load_game_state_callback(unsigned char *buffer, int len)
{
   // TODO call retro_unserialize
   // memcpy(&gs, buffer, len);
   return true;
}

bool __cdecl
net_save_game_state_callback(unsigned char **buffer, int *len, int *checksum, int frame)
{
   // *len = sizeof(gs);
   // *buffer = (unsigned char *)malloc(*len);
   // if (!*buffer) {
   //    return false;
   // }
   // // todo change this line
   // memcpy(*buffer, &gs, *len);
   // *checksum = fletcher32_checksum((short *)*buffer, *len / 2);
   return true;
}

void __cdecl 
net_free_buffer(void *buffer)
{
   free(buffer);
}

bool __cdecl
net_on_event_callback(GGPOEvent *info)
{
   int progress;
   switch (info->code) {
   case GGPO_EVENTCODE_CONNECTED_TO_PEER:
      ngs.SetConnectState(info->u.connected.player, Synchronizing);
      break;
   case GGPO_EVENTCODE_SYNCHRONIZING_WITH_PEER:
      progress = 100 * info->u.synchronizing.count / info->u.synchronizing.total;
      ngs.UpdateConnectProgress(info->u.synchronizing.player, progress);
      break;
   case GGPO_EVENTCODE_SYNCHRONIZED_WITH_PEER:
      ngs.UpdateConnectProgress(info->u.synchronized.player, 100);
      break;
   case GGPO_EVENTCODE_RUNNING:
      ngs.SetConnectState(Running);
      // renderer->SetStatusText("");
      break;
   case GGPO_EVENTCODE_CONNECTION_INTERRUPTED:
      ngs.SetDisconnectTimeout(info->u.connection_interrupted.player,
                               Platform::GetCurrentTimeMS(),
                               info->u.connection_interrupted.disconnect_timeout);
      break;
   case GGPO_EVENTCODE_CONNECTION_RESUMED:
      ngs.SetConnectState(info->u.connection_resumed.player, Running);
      break;
   case GGPO_EVENTCODE_DISCONNECTED_FROM_PEER:
      ngs.SetConnectState(info->u.disconnected.player, Disconnected);
      break;
   case GGPO_EVENTCODE_TIMESYNC:
      usleep(1000 * info->u.timesync.frames_ahead / 60);
      break;
   }
   return true;
}


bool __cdecl
net_log_game_state(char *filename, unsigned char *buffer, int len)
{
   return true;
}