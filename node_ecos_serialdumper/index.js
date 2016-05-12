/*
 * Copyright 2016 Smx (smxdev4@gmail.com)
 * !! This code is not in the best shape !!
 */
if(process.argv.length < 3){
	console.error("== EcOS firmware dumper via serial console (Linksys EPC3208G) ==");
	console.error("== Edit the source code for other models ==");
	console.error("> Usage: " + process.argv[1] + " [serial port device]");
	process.exit(1);
}

const serialport = require("serialport");
const _ = require("lodash");
const fs = require("fs");
const util = require("util");
var sport = new serialport.SerialPort(process.argv[2], {
	baudrate: 115200,
	parser: serialport.parsers.readline('\n')
});

var EventEmitter = require('events').EventEmitter;

var Lock = function(){
	this.rts = true;
}
util.inherits(Lock, EventEmitter);

var curno = 0;
var	bytesWritten = 0;

/* Command line is "CM>" */
const SHELL_MARKER = "CM";
const sections = require("./flashmap_EPC3208G");

/* Ready to Send Lock */
var Lock_Rts = new Lock();

sport.on("open", function(){
	console.log("It works!");
	main(sport);
});
sport.on("data", function(data){
	parser(data);
});

Array.prototype.clean = function() {
  for (var i = 0; i < this.length; i++) {
    if (this[i].trim().length <= 0) {         
      this.splice(i, 1);
      i--;
    }
  }
  return this;
};


const states = {
	IDLE: 0,
	READ_MEMORY: 1,
	READ_FLASH: 2,
};

var current_state = states.IDLE;
var currentFile;

function comwrite(data){
	sport.write(data + "\n", function(){
		sport.drain();
	});
}

function isPrompt(str){
	if(str.indexOf(SHELL_MARKER) >= 0)
		return true;
	return false;
}

function rmem_parser(data){
	var parts = data.trim().split(" ");
	parts = parts.clean();
	if(parts.length <= 0 || isPrompt(parts[0])){
		console.log(data);
		return;
	}
	var strPartIdx = parts.indexOf("|");
	if(strPartIdx >= 0){
		parts.splice(strPartIdx, parts.length - strPartIdx);
	}
	console.log(parts);
}

function rflash_parser(data){
	var parts = data.trim().split(" ");
	parts = parts.clean();
	if(parts.length <= 0 || isPrompt(parts[0]) || parts[0].length != 8){
		return;
	}
	var hex = new Buffer( parts.join(""), "hex");
	fs.appendFileSync("./"+currentFile+".bin", hex, "binary", function(err){
		if(err)
			console.log("ERROR: "+err);
	});
	console.log(hex);
	Lock_Rts.emit("ready");
}

function parser(data){
	switch(current_state){
		case states.READ_MEMORY:
			return rmem_parser(data);
		case states.READ_FLASH:
			return rflash_parser(data);
		case states.IDLE:
		default:
			//console.log(data);
			break;
	}
}

function nextSection(){
	var name = Object.keys(sections)[curno];
	console.log("Switching to region "+name);
	comwrite("cd /flash");

	comwrite("close");
	comwrite("deinit");
	comwrite("init");
	comwrite("open "+name);

	bytesWritten = 0;
	
	console.log("Ready to dump "+name);
	currentFile = name;

	Lock_Rts.emit("ready");
}

function setupEvents(com){
	current_state = states.READ_FLASH;
	Lock_Rts.on("ready", function(){
		console.log("Sending Read at offset "+bytesWritten+"...");
		/* Issue a 'read 4 8192' command, and parse its result in the event handler */ 
		var cmd = "read 4 8192 "+bytesWritten.toString();
		console.log(cmd);
		comwrite(cmd);
		bytesWritten += 8192;
		if(bytesWritten > sections[Object.keys(sections)[curno]]){
			curno++;
			nextSection();
			if(curno >= Object.keys(sections).length)
				process.exit(1);
		}
	})
}

function main(com){
	setupEvents(com);
	nextSection();
}