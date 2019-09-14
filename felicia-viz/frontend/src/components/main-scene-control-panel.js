import { inject, observer } from 'mobx-react';
import PropTypes from 'prop-types';
import React, { Component } from 'react';
import { Form } from '@streetscape.gl/monochrome';

import {
  POSEF_WITH_TIMESTAMP_MESSAGE,
  POSE3F_WITH_TIMESTAMP_MESSAGE,
} from '@felicia-viz/proto/messages/geometry';
import {
  OCCUPANCY_GRID_MAP_MESSAGE,
  POINTCLOUD_MESSAGE,
} from '@felicia-viz/proto/messages/map-message';
import { TopicList } from '@felicia-viz/ui';
import { FORM_STYLE } from '@felicia-viz/ui/custom-styles';

@inject('store')
@observer
export default class MainSceneControlPanel extends Component {
  static propTypes = {
    store: PropTypes.object.isRequired,
  };

  SETTINGS = {
    header: { type: 'header', title: 'MainScene Control' },
    sectionSeperator: { type: 'separator' },
    mapTopics: {
      type: 'custom',
      render: self => {
        return (
          <TopicList
            {...self}
            typeNames={[
              OCCUPANCY_GRID_MAP_MESSAGE,
              POINTCLOUD_MESSAGE,
              POSEF_WITH_TIMESTAMP_MESSAGE,
              POSE3F_WITH_TIMESTAMP_MESSAGE,
            ]}
          />
        );
      },
    },
  };

  _onChange = values => {}; // eslint-disable-line no-unused-vars

  render() {
    return <Form data={this.SETTINGS} values={{}} style={FORM_STYLE} onChange={this._onChange} />;
  }
}
