package com.example.sudokusolver

import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.SimpleExpandableListAdapter
import androidx.fragment.app.Fragment
import com.example.sudokusolver.databinding.FragmentHelpBinding

class HelpFragment : Fragment() {
    private lateinit var faqListAdapter: SimpleExpandableListAdapter

    private lateinit var binding: FragmentHelpBinding

    private var _lastExpandedPos = -1

    override fun onCreateView(inflater: LayoutInflater, container: ViewGroup?, savedInstanceState: Bundle?): View? {
        binding = FragmentHelpBinding.inflate(inflater, container, false)
        return binding.root
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        val questions = resources.getStringArray(R.array.faq_questions)
        val answers = resources.getStringArray(R.array.faq_answers)

        val groupList = questions.map { mapOf("QUESTION" to it) }
        val childList = answers.map { mutableListOf(mutableMapOf("ANSWER" to it)) }.toMutableList()

        faqListAdapter = SimpleExpandableListAdapter(
            requireContext(),
            groupList,
            android.R.layout.simple_expandable_list_item_1,
            arrayOf("QUESTION"),
            intArrayOf(android.R.id.text1),
            childList,
            android.R.layout.simple_list_item_1,
            arrayOf("ANSWER"),
            intArrayOf(android.R.id.text1),
        )

        binding.helpFaqList.setAdapter(faqListAdapter)
        binding.helpFaqList.setOnGroupExpandListener {
            if (_lastExpandedPos != -1 && it != _lastExpandedPos)
                binding.helpFaqList.collapseGroup(_lastExpandedPos)
            _lastExpandedPos = it
        }
    }
}