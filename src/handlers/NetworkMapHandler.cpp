#ifndef NETWORKMAP_HANDLER
#define NETWORKMAP_HANDLER

#include "Globals.h"

class NetworkMapHandler {
	public:
		static void 	Pool();
		static void 	SetLastInfoReceivedTimer();
};

void IRAM_ATTR NetworkMapHandler::Pool() {
	if (Network.PoolingNetworkMapReceivedTimer == Settings.Pooling.NetworkMap.DefaultValue)
		return;

	if (Network.PoolingNetworkMapReceivedTimer == 0) {
		Network.PoolingNetworkMapReceivedTimer = Settings.Pooling.NetworkMap.DefaultValue;
		NetworkSync::Start();
		return;
	}

	if (Network.PoolingNetworkMapReceivedTimer > 0) {
		Network.PoolingNetworkMapReceivedTimer--;
		return;
	}
}

#endif
