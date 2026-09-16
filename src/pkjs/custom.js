module.exports=function() {
  this.on(this.EVENTS.AFTER_BUILD,function() {
    var style=document.createElement('style');
    style.textContent='.component-submit{position:sticky;bottom:0;background:#fff;z-index:10;padding:12px!important}.component-submit button{min-height:56px;font-size:20px;width:100%}.component-color .picker{max-width:36rem}.component-color .picker-wrap{padding:8px}.component-color .value{min-width:72px;min-height:44px}';
    document.head.appendChild(style);
  });
};
