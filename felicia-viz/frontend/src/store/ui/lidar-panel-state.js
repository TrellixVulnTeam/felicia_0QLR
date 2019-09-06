import { observable, action } from 'mobx';

import TopicSubscribable from 'store/topic-subscribable';

export class LidarFrameMessage {
  constructor(message) {
    const {
      angleStart,
      angleEnd,
      angleDelta,
      timeDelta,
      scanTime,
      rangeMin,
      rangeMax,
      ranges,
      intensities,
      timestamp,
    } = message.data;
    this.angleStart = angleStart;
    this.angleEnd = angleEnd;
    this.angleDelta = angleDelta;
    this.timeDelta = timeDelta;
    this.scanTime = scanTime;
    this.rangeMin = rangeMin;
    this.rangeMax = rangeMax;
    this.ranges = ranges;
    this.intensities = intensities;
    this.timestamp = timestamp;
  }
}

export default class LidarPanelState extends TopicSubscribable {
  @observable frame = null;

  @action update(message) {
    this.frame = new LidarFrameMessage(message);
  }

  type = () => {
    return 'LidarPanel';
  };
}
