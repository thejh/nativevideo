// ==UserScript==
// @name           Native Video Player
// @description    Play videos using mplayer (optionally audio-only)
// @author         Jann Horn
// @include        http://www.youtube.com/watch?*
// @version        1.0
// ==/UserScript==

var password = 'xxxxxxxx'

function handle_vid(v) {
  if (!confirm("go native?")) return
  v.pause()
  var url = v.src
  // the destination will return "no content", thereby preserving the youtube page
  var modeprefix = confirm("display video?")?'':'!'
  document.location = 'http://localhost/cgi-bin/nativevideo?'+password+'-'+modeprefix+url;
}

function tryit() {
  var cur_vids = document.getElementsByTagName('video')
  if (cur_vids.length == 0) {
    console.log('no video present yet when I looked')
    return false
  } else if (!cur_vids[0].src) {
    console.log('video present, but without src! waiting for src...')
    var observer2 = new WebKitMutationObserver(function(mutations) {
      console.log("attribute observer triggered")
      if (tryit()) observer2.disconnect()
    })
    observer2.observe(cur_vids[0], {attributes: true})
    return true
  } else {
    console.log("video is present! src="+cur_vids[0].src)
    handle_vid(cur_vids[0])
    return true
  }
}

if (!tryit()) {
  var vcont = document.getElementsByClassName("html5-video-container")[0]
  console.log("registering observer...")
  var observer1 = new WebKitMutationObserver(function(mutations) {
    console.log("childnode observer triggered")
    if (tryit()) observer1.disconnect()
  })
  observer1.observe(vcont, {childList: true})
  //console.log(vcont)
  console.log("observer registered")
}
