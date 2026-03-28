const { connect } = require('./build/Release/samp');
connect(message);

function message(msg) {
    console.log('JAVASCRIPT MSG', msg);
}
