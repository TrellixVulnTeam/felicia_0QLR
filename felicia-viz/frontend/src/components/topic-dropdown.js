import React, { Component } from 'react';
import PropTypes from 'prop-types';
import { toJS } from 'mobx';
import { inject, observer } from 'mobx-react';
import { Dropdown, Label } from '@streetscape.gl/monochrome';

import PanelItemContainer from 'components/panel-item-container';
import { FeliciaVizStore } from 'store';
import { failedToFindActiveState } from 'util/error';
import META_INFO_SUBSCRIBER from 'util/meta-info-subscriber';
import QUERY_TYPE from 'common/query-type';

@inject('store')
@observer
export default class TopicDropdown extends Component {
  static propTypes = {
    title: PropTypes.string.isRequired,
    typeName: PropTypes.string.isRequired,
    store: PropTypes.instanceOf(FeliciaVizStore).isRequired,
  };

  componentDidMount() {
    META_INFO_SUBSCRIBER.request(QUERY_TYPE.Topics.name);
  }

  _onTopicChange = value => {
    const { store } = this.props;
    const state = store.uiState.activeWindow.getState();
    if (state) {
      state.selectTopic(value);
    } else {
      failedToFindActiveState();
    }
  };

  render() {
    const { title, typeName, store } = this.props;
    const state = store.uiState.activeWindow.getState();
    let value = '';
    if (state) {
      value = state.topic;
    } else {
      failedToFindActiveState();
    }

    const topics = toJS(store.metaInfo.topics);
    const data = topics.reduce((obj, v) => {
      const { topic } = v;
      if (typeName === v.typeName) {
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
        <Dropdown value={value} data={data} onChange={this._onTopicChange} />
      </PanelItemContainer>
    );
  }
}
