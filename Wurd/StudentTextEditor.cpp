#include "StudentTextEditor.h"
#include "Undo.h"
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <algorithm>
using namespace std;

TextEditor* createTextEditor(Undo* un)
{
	return new StudentTextEditor(un);
}

StudentTextEditor::StudentTextEditor(Undo* undo)
 : TextEditor(undo), m_row(0), m_col(0), m_numLines(1) {
	m_lines.push_back("");
	m_curLine = m_lines.begin();
	// TODO
}

StudentTextEditor::~StudentTextEditor()
{
	// TODO
}

void StudentTextEditor::resetCursor() {
	m_curLine = m_lines.begin();
	m_row = 0;
	m_col = 0;
}

// TODO: Bug both here and in sample when loading empty file
bool StudentTextEditor::load(std::string file) {
	ifstream infile(file);
	if (!infile)
		return false;
	reset();
	string line;
	while (getline(infile, line)) {
		line.erase(remove_if(line.begin(), line.end(), [](char ch) { return ch == '\r' || ch == '\n'; }), line.end());
		m_lines.push_back(line);
		m_numLines++;
	}
	resetCursor();
	return true;
}

bool StudentTextEditor::save(std::string file) {
	ofstream outfile(file);
	if (!outfile)
		return false;
	for (auto it = m_lines.begin(); it != m_lines.end(); ++it)
		outfile << *it << '\n';
	return true;
}

void StudentTextEditor::reset() {
	m_lines.clear();
	resetCursor();
	m_numLines = 0;
	getUndo()->clear();
}

void StudentTextEditor::insertAtCursor(char ch) {
	m_curLine->insert(m_col, 1, ch);
	m_col++;
}

void StudentTextEditor::insert(char ch) {
	if (ch == '\t') {
		for (int i = 0; i < 4; ++i) {
			getUndo()->submit(Undo::Action::INSERT, m_row, m_col, ' ');
			insertAtCursor(' ');
		}
	}
	else {
		getUndo()->submit(Undo::Action::INSERT, m_row, m_col, ch);
		insertAtCursor(ch);
	}
}

void StudentTextEditor::splitAtCursor() {
	auto curLineItr = m_curLine;
	string newLine = curLineItr->substr(m_col);
	curLineItr->erase(m_col);
	m_curLine = m_lines.insert(++curLineItr, newLine);
}

void StudentTextEditor::enter() {
	splitAtCursor();
	getUndo()->submit(Undo::Action::SPLIT, m_row, m_col);
	m_row++;
	m_col = 0;
	m_numLines++;
}

char StudentTextEditor::eraseAtCursor() {
	char deletedChar = m_curLine->at(m_col);
	m_curLine->erase(m_col, 1);
	return deletedChar;
}

void StudentTextEditor::joinAtCursor() {
	auto nextLineItr = m_curLine;
	string nextLine = *(++nextLineItr);
	m_curLine->append(nextLine);
	m_lines.erase(nextLineItr);
	m_numLines--;
}

void StudentTextEditor::del() {
	if (m_col < m_curLine->size()) {
		char deletedChar = eraseAtCursor();
		getUndo()->submit(Undo::Action::DELETE, m_row, m_col, deletedChar);
	}
	else if (m_row < m_numLines - 1) {
		joinAtCursor();
		getUndo()->submit(Undo::Action::JOIN, m_row, m_col);
	}
}

void StudentTextEditor::backspace() {
	if (m_col > 0) {
		m_col--;
		char deletedChar = eraseAtCursor();
		getUndo()->submit(Undo::Action::DELETE, m_row, m_col, deletedChar);
	}
	else if (m_row > 0) {
		m_curLine--;
		m_row--;
		m_col = m_curLine->size();
		joinAtCursor();
		getUndo()->submit(Undo::Action::JOIN, m_row, m_col);
	}
}

void StudentTextEditor::move(Dir dir) {
	switch (dir) {
	case Dir::UP:
		if (m_row > 0) {
			m_curLine--;
			m_row--;
			if (m_col > m_curLine->size())
				m_col = m_curLine->size();
		}
		break;
	case Dir::DOWN:
		if (m_row < m_numLines - 1) {
			m_curLine++;
			m_row++;
			if (m_col > m_curLine->size())
				m_col = m_curLine->size();
		}
		break;
	case Dir::LEFT:
		if (m_col > 0)
			m_col--;
		else if (m_row > 0) {
			m_curLine--;
			m_row--;
			m_col = m_curLine->size();
		}
		break;
	case Dir::RIGHT:
		if (m_col < m_curLine->size())
			m_col++;
		else if (m_row < m_numLines - 1) {
			m_curLine++;
			m_row++;
			m_col = 0;
		}
		break;
	case Dir::HOME:
		m_col = 0;
		break;
	case Dir::END:
		m_col = m_curLine->size();
		break;
	}
}

void StudentTextEditor::getPos(int& row, int& col) const {
	row = m_row;
	col = m_col;
}

void StudentTextEditor::moveToRow(list<string>::iterator& it, int& row, int targetRow) const {
	if (row > targetRow) {
		while (row > targetRow) {
			it--;
			row--;
		}
	}
	else {
		while (row < targetRow) {
			it++;
			row++;
		}
	}
}

int StudentTextEditor::getLines(int startRow, int numRows, std::vector<std::string>& lines) const {
	if (startRow < 0 || numRows < 0 || startRow > m_numLines)
		return -1;

	auto startRowItr = m_curLine;
	int curRow = m_row;
	moveToRow(startRowItr, curRow, startRow);

	lines.clear();

	int linesCopied = 0;
	while (startRowItr != m_lines.end() && curRow < m_numLines && linesCopied < numRows) {
		lines.push_back(*startRowItr);
		startRowItr++;
		curRow++;
		linesCopied++;
	}

	return linesCopied;
}

void StudentTextEditor::moveCursor(int row, int col) {
	moveToRow(m_curLine, m_row, row);
	if (col > m_curLine->size())
		m_col = m_curLine->size();
	else
		m_col = col;
}

void StudentTextEditor::undo() {
	int row = m_row;
	int col = m_col;
	int count = 1;
	string text;
	switch (getUndo()->get(row, col, count, text)) {
	case Undo::Action::INSERT:
		moveCursor(row, col);
		for (auto it = text.begin(); it != text.end(); ++it)
			insertAtCursor(*it);
		break;
	case Undo::Action::DELETE:
		moveCursor(row, col);
		for (int i = 0; i < count; ++i)
			eraseAtCursor();
		break;
	case Undo::Action::SPLIT:
		moveCursor(row, col);
		splitAtCursor();
		break;
	case Undo::Action::JOIN:
		moveCursor(row, col);
		joinAtCursor();
		break;
	}
	moveCursor(row, col);
}
