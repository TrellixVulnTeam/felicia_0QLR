import { toJS } from 'mobx';
import { inject, observer } from 'mobx-react';
import PropTypes from 'prop-types';
import React, { Component } from 'react';
import { Dropdown, Label } from '@streetscape.gl/monochrome';

import { findWSChannel, hasWSChannel } from '@felicia-viz/communication/subscriber';

import { PanelItemContainer } from './panel-item';

@inject('store')
@observer
export default class TopicDropdown extends Component {
  static propTypes = {
    title: PropTypes.string.isRequired,
    typeName: PropTypes.string.isRequired,
    store: PropTypes.object.isRequired,
    isEnabled: PropTypes.bool.isRequired,
  };

  _onTopicChange = value => {
    const { typeName, store } = this.props;
    const topics = toJS(store.topicInfo.topics);
    let found = topics.find(v => {
      return v.topic === value;
    });
    if (!found) {
      console.error(`Failed to find topic ${value}`);
      return;
    }

    found = findWSChannel(found);

    if (found) {
      const viewState = store.uiState.activeViewState.getState();
      viewState.setTopic(typeName, value, found.ipEndpoint);
    } else {
      console.error(`Failed to find ip endpoint for ${value}`);
    }
  };

  render() {
    const { title, typeName, store, isEnabled } = this.props;
    const viewState = store.uiState.activeViewState.getState();
    let value = '';
    if (viewState.topics.has(typeName)) {
      value = viewState.topics.get(typeName);
    }

    const topics = toJS(store.topicInfo.topics);
    const data = topics.reduce((obj, v) => {
      const { topic } = v;
      if (hasWSChannel(v) && typeName === v.typeName) {
        obj[topic] = topic; // eslint-disable-line no-param-reassign
      }
      return obj;
    }, {});
    if (value === '') {
      data[''] = '';
    }

    return (
      <PanelItemContainer>
        <Label>{title}</Label>
        <Dropdown value={value} data={data} onChange={this._onTopicChange} isEnabled={isEnabled} />
      </PanelItemContainer>
    );
  }
}
