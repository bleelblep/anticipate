// One in-flight AppMessage for settings and weather, with bounded retries.
var queue=[], active=false;
function pump() {
  if(active || !queue.length) return;
  active=true;
  var item=queue[0], settled=false;
  function finish(ok, error) {
    if(settled) return;
    settled=true;active=false;
    if(!ok && ++item.attempts<3) {
      active=true;
      setTimeout(function(){active=false;pump();},item.attempts*1000);
      return;
    }
    queue.shift();
    console.log(item.kind+(ok?' delivered':' failed: '+JSON.stringify(error)));
    if(ok && item.done) item.done();
    pump();
  }
  try {Pebble.sendAppMessage(item.data,function(){finish(true);},function(e){finish(false,e);});}
  catch(e) {finish(false,String(e));}
}
module.exports.send=function(kind,data,done) {
  // Keep the newest unsent update of each kind.
  for(var i=queue.length-1;i>0;i--) if(queue[i].kind===kind) queue.splice(i,1);
  queue.push({kind:kind,data:data,done:done,attempts:0});pump();
};
