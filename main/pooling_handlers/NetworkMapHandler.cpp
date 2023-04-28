#include "HandlersPooling.h"

void HandlersPooling_t::NetworkMapHandler::Pool() {
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