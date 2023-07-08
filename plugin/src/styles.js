
export default `
.client-headline {
  display: inline-block;
  max-width: 100%;
  padding: 0;
  margin: 0;
  font-size: 14px;
  font-weight: normal;
  line-height: 22px;
  color: #c5d6d4;
  white-space: nowrap;
  text-overflow: ellipsis;
  overflow: hidden;
}

.client-item {
  display: block;
  line-height: 15px;
  white-space: nowrap;
  text-overflow: ellipsis;
  overflow: hidden;
}

.client-item.nickname {
  font-size: 15px;
  color: #b3dfff;
}

.client-item.status {
  font-size: 12px;
  color: #4cb4ff;
}

.client-avatar {
  margin: 0;
  width: 32px;
  height: 32px;
  border-right: 2px solid #6dcff6;
}

.icon-button {
  padding: 7px 10px 4px !important;
  width: auto !important;
  min-width: 0 !important;
  flex-shrink: 0;
}`;
