import { observable, action } from 'mobx';

import { ImageWithHumansMessage } from 'messages/image-with-humans';
import TopicSubscribable from 'store/topic-subscribable';

export default class ImageWithHumansViewState extends TopicSubscribable {
  @observable frame = null;

  @observable threshold = 0.3;

  @action update(message) {
    this.frame = new ImageWithHumansMessage(message.data);
  }

  @action setThreshold(newThreshold) {
    this.threshold = newThreshold;
  }

  viewType = () => {
    return 'ImageWithHumansView';
  };
}
